// src/chat_node.rs

use anyhow::{Context, Result};
use base64::engine::general_purpose::STANDARD;
use base64::Engine;
use hex;
use iroh::{
    endpoint::{Connection, Endpoint, SendStream},
    protocol::{AcceptError, ProtocolHandler, Router},
    NodeId, SecretKey,
};
use serde_json::json;
use tokio::io::{AsyncReadExt, BufReader};
use tokio::sync::broadcast;
use crate::android_glue::SEND_STREAM;

#[cfg(target_os = "android")]
use crate::android_glue::{JAVA_LISTENER, JVM};
#[cfg(target_os = "android")]
use jni::objects::{JValue, JObject};

#[derive(Debug, Clone)]
pub struct ChatNode {
    router: Router,
    accept_tx: broadcast::Sender<AcceptEvent>,
}

impl ChatNode {
    pub fn event_receiver(&self) -> broadcast::Receiver<AcceptEvent> {
        self.accept_tx.subscribe()
    }

    pub async fn spawn(predefined_hex_secret: &str) -> Result<Self> {
        let sk_bytes = hex::decode(predefined_hex_secret)
            .context("Failed to decode secret key hex")?;
        let sk_bytes: &[u8; 32] = sk_bytes
            .as_slice()
            .try_into()
            .context("Secret key must be 32 bytes")?;
        let secret_key = SecretKey::from_bytes(sk_bytes);

        let endpoint = Endpoint::builder()
            .secret_key(secret_key)
            .discovery_n0()
            .alpns(vec![Chat::ALPN.to_vec()])
            .bind()
            .await?;
        log::info!("This node ID: {}", endpoint.node_id());

        let (tx, _) = broadcast::channel(128);
        let handler = Chat::new(tx.clone(), endpoint.clone());
        let router = Router::builder(endpoint.clone())
            .accept(Chat::ALPN, handler)
            .spawn();

        Ok(Self { router, accept_tx: tx })
    }

    pub fn endpoint(&self) -> &Endpoint {
        self.router.endpoint()
    }

    /// Send one packet with a 4-byte BE length prefix, then the raw bytes.
    pub async fn send_packet(&self, packet: &[u8]) -> Result<()> {
        let send_mutex = SEND_STREAM
            .get()
            .context("SendStream not initialized")?;
        let mut tx = send_mutex.lock().await;

        // 1) length prefix (u32 BE)
        let len_bytes = (packet.len() as u32).to_be_bytes();
        tx.write_all(&len_bytes).await?;
        // 2) the payload
        tx.write_all(packet).await?;

        Ok(())
    }

    /// Connect outbound, then spawn a task that:
    ///   • reads a 4-byte length
    ///   • reads exactly that many bytes
    ///   • base64-encodes and forwards to Kotlin
    pub async fn connect_and_chat(&self, peer: NodeId) -> Result<SendStream> {
        log::info!("Connecting to peer: {}", peer);
        let conn = self.endpoint().connect(peer, Chat::ALPN).await?;
        let (send, recv) = conn.open_bi().await?;

        tokio::spawn({
            let mut reader = BufReader::new(recv);
            let peer = peer.clone();
            async move {
                loop {
                    // 1) read length header
                    let mut len_buf = [0u8; 4];
                    if let Err(e) = reader.read_exact(&mut len_buf).await {
                        log::error!("Error reading length header from {}: {:?}", peer, e);
                        break;
                    }
                    let payload_len = u32::from_be_bytes(len_buf) as usize;

                    // 2) read the payload
                    let mut buf = vec![0u8; payload_len];
                    if let Err(e) = reader.read_exact(&mut buf).await {
                        log::error!("Error reading payload ({} bytes) from {}: {:?}", payload_len, peer, e);
                        break;
                    }

                    // 3) encode & dispatch
                    let payload_b64 = STANDARD.encode(&buf);
                    let js = json!({
                        "type":        "message",
                        "peer":        peer.to_string(),
                        "payload_b64": payload_b64,
                    })
                    .to_string();
                    log::info!("> [{}] {}", peer, js);

                    #[cfg(target_os = "android")]
                    if let (Some(vm), Some(listener)) = (JVM.get(), JAVA_LISTENER.get()) {
                        if let Ok(mut env) = vm.attach_current_thread() {
                            if let Ok(java_bytes) = env.byte_array_from_slice(js.as_bytes()) {
                                let jobj = JObject::from(java_bytes);
                                let _ = env.call_method(
                                    listener.as_obj(),
                                    "onIrohMessage",
                                    "([B)V",
                                    &[JValue::Object(&jobj)],
                                );
                            }
                        }
                    }
                }
                log::info!("Chat ended with peer: {}", peer);
            }
        });

        Ok(send)
    }
}

#[derive(Debug, Clone)]
pub enum AcceptEvent {
    Accepted { node_id: NodeId },
    Closed   { node_id: NodeId, error: Option<String> },
}

#[derive(Debug, Clone)]
pub struct Chat {
    event_sender: broadcast::Sender<AcceptEvent>,
    endpoint:     Endpoint,
}

impl Chat {
    pub const ALPN: &[u8] = b"iroh/example-chat/0";

    pub fn new(event_sender: broadcast::Sender<AcceptEvent>, endpoint: Endpoint) -> Self {
        Self { event_sender, endpoint }
    }

    /// Handles incoming connect requests: same length-prefix logic as above
    async fn handle_connection(&self, connection: Connection) -> Result<(), AcceptError> {
        let peer = connection.remote_node_id()?;
        let _ = self.event_sender.send(AcceptEvent::Accepted { node_id: peer });
        log::info!("Incoming connection from: {}", peer);

        tokio::spawn({
            let tx = self.event_sender.clone();
            async move {
                match connection.accept_bi().await {
                    Ok((_send, recv)) => {
                        let mut reader = BufReader::new(recv);
                        loop {
                            // length header
                            let mut len_buf = [0u8; 4];
                            if let Err(e) = reader.read_exact(&mut len_buf).await {
                                log::error!("Error reading length header from {}: {:?}", peer, e);
                                break;
                            }
                            let payload_len = u32::from_be_bytes(len_buf) as usize;

                            // payload
                            let mut buf = vec![0u8; payload_len];
                            if let Err(e) = reader.read_exact(&mut buf).await {
                                log::error!("Error reading payload ({} bytes) from {}: {:?}", payload_len, peer, e);
                                break;
                            }

                            // dispatch
                            let payload_b64 = STANDARD.encode(&buf);
                            let js = json!({
                                "type":        "message",
                                "peer":        peer.to_string(),
                                "payload_b64": payload_b64,
                            })
                            .to_string();
                            log::info!("> [{}] {}", peer, js);

                            #[cfg(target_os = "android")]
                            if let (Some(vm), Some(listener)) = (JVM.get(), JAVA_LISTENER.get()) {
                                if let Ok(mut env) = vm.attach_current_thread() {
                                    if let Ok(java_bytes) = env.byte_array_from_slice(js.as_bytes()) {
                                        let jobj = JObject::from(java_bytes);
                                        let _ = env.call_method(
                                            listener.as_obj(),
                                            "onIrohMessage",
                                            "([B)V",
                                            &[JValue::Object(&jobj)],
                                        );
                                    }
                                }
                            }
                        }
                        log::info!("Accepted chat ended with peer: {}", peer);
                    }
                    Err(e) => {
                        log::error!("Failed to accept bi-stream from {}: {:?}", peer, e);
                    }
                }
            }
        });

        Ok(())
    }
}

impl ProtocolHandler for Chat {
    async fn accept(&self, connection: Connection) -> Result<(), AcceptError> {
        self.clone().handle_connection(connection).await
    }
}
