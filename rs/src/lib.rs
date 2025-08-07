// src/lib.rs

pub mod chat_node;
pub mod android_glue;

use android_logger::Config;
use jni::objects::{JByteArray, JClass, JObject, JString};
use jni::sys::{jboolean, jbyteArray};
use jni::JNIEnv;
use log::LevelFilter;
use once_cell::sync::OnceCell;
use std::sync::Mutex;
use tokio::runtime::Runtime;
use tokio::sync::{broadcast, Mutex as TokioMutex};

use crate::chat_node::{AcceptEvent, ChatNode};

static RT: OnceCell<Runtime> = OnceCell::new();
static NODE: OnceCell<ChatNode> = OnceCell::new();
static EVENTS: OnceCell<Mutex<broadcast::Receiver<AcceptEvent>>> = OnceCell::new();
use crate::android_glue::SEND_STREAM;
use android_logger::FilterBuilder;

/// JNI: IrohBridge.start_node(secretHex: String): Boolean
#[no_mangle]
pub extern "system" fn Java_nz_scuttlebutt_tremolavossbol_tssb_iroh_IrohBridge_start_1node(
    mut env: JNIEnv,
    _class: JClass,
    secret_hex: JString,
) -> jboolean {
    android_logger::init_once(
    Config::default()
        .with_tag("RustIroh")
        .with_max_level(LevelFilter::Error)
        .with_filter(
            FilterBuilder::new()
                .parse("RustIroh=trace,iroh=info")
                .build(),
        ),
    );

    log::info!("Starting Iroh node…");

    let hex = match env.get_string(&secret_hex) {
        Ok(java_str) => match java_str.to_str() {
            Ok(s) => s.to_owned(),
            Err(e) => {
                log::error!("Invalid UTF-8 in secret hex: {:?}", e);
                return 0;
            }
        },
        Err(e) => {
            log::error!("Failed to get secret hex: {:?}", e);
            return 0;
        }
    };

    let rt = RT.get_or_init(|| Runtime::new().unwrap());
    match rt.block_on(ChatNode::spawn(&hex)) {
        Ok(node) => {
            NODE.set(node.clone()).ok();
            let rx = node.event_receiver();
            EVENTS.set(Mutex::new(rx)).ok();
            1
        }
        Err(e) => {
            log::error!("Failed to spawn ChatNode: {:?}", e);
            0
        }
    }
}

/// JNI: IrohBridge.stop_node(): Boolean
#[no_mangle]
pub extern "system" fn Java_nz_scuttlebutt_tremolavossbol_tssb_iroh_IrohBridge_stop_1node(
    _env: JNIEnv,
    _class: JClass
) -> jboolean {
    log::info!("Stopping Iroh node…");

    let node = match NODE.get() {
        Some(n) => n.clone(),
        None => {
            log::error!("JNI stop: NODE not initialized (start_node never called?)");
            return 0;
        }
    };
    node.endpoint().close();

    return 1;
}

/// JNI: IrohBridge.connect(peerHex: String): Boolean
#[no_mangle]
pub extern "system" fn Java_nz_scuttlebutt_tremolavossbol_tssb_iroh_IrohBridge_connectAndChat(
    mut env: JNIEnv,
    _class: JClass,
    peer_hex: JString,
) -> jboolean {
    // 1) Get the string
    let peer_hex_str = match env.get_string(&peer_hex) {
        Ok(java_str) => {
            let s = java_str.to_string_lossy().into_owned();
            log::info!("JNI connectAndChat: got peer_hex = {}", s);
            s
        }
        Err(e) => {
            log::error!("JNI connectAndChat: env.get_string failed: {:?}", e);
            return 0;
        }
    };

    // 2) Parseg NodeId
    let peer_id = match peer_hex_str.parse() {
        Ok(id) => id,
        Err(e) => {
            log::error!(
                "JNI connectAndChat: failed to parse NodeId from '{}': {:?}",
                peer_hex_str,
                e
            );
            return 0;
        }
    };

    // 3) Ensure the node is up
    let node = match NODE.get() {
        Some(n) => n.clone(),
        None => {
            log::error!("JNI connectAndChat: NODE not initialized (start_node never called?)");
            return 0;
        }
    };

    // 4) Actually *block* on connect_and_chat
    let rt = RT.get().unwrap();
    match rt.block_on(node.connect_and_chat(peer_id)) {
        Ok(send) => {
            SEND_STREAM.set(TokioMutex::new(send)).ok();
            log::info!("JNI connectAndChat: successfully connected to {}", peer_hex_str);
            1
        }
        Err(e) => {
            log::error!("JNI connectAndChat: connect_and_chat error: {:?}", e);
            0
        }
    }
}

/// JNI: IrohBridge.disconnect(): Boolean
#[no_mangle]
pub extern "system" fn Java_nz_scuttlebutt_tremolavossbol_tssb_iroh_IrohBridge_disconnect(
    mut env: JNIEnv,
    _class: JClass
) -> jboolean {
    // close()
    return 1
}

/*
/// JNI: IrohBridge.tryRecv(): String
#[no_mangle]
pub extern "system" fn Java_nz_scuttlebutt_tremolavossbol_tssb_iroh_IrohBridge_tryRecv<'a>(
    mut env: JNIEnv<'a>,
    _class: JClass,
) -> JString<'a> {
    // pull out the receiver guard
    let mut rx = match EVENTS.get().and_then(|m| m.lock().ok()) {
        Some(r) => r,
        None => return env.new_string("").unwrap(),
    };

    let out = match rx.try_recv() {
        Ok(AcceptEvent::Accepted { node_id }) => {
            json!({ "type": "accepted", "peer": node_id.to_string() }).to_string()
        }
        Ok(AcceptEvent::Closed { node_id, error }) => {
            json!({
                "type": "closed",
                "peer": node_id.to_string(),
                "error": error.unwrap_or_default()
            })
            .to_string()
        }
        Err(_) => String::new(),
    };

    env.new_string(out).unwrap()
}
*/

/// JNI: IrohBridge.get_node_id(): String
#[no_mangle]
pub extern "system" fn Java_nz_scuttlebutt_tremolavossbol_tssb_iroh_IrohBridge_getNodeId<'a>(
    mut env: JNIEnv<'a>,
    _class: JClass,
) -> JString<'a> {
    if let Some(n) = NODE.get() {
        let id = n.endpoint().node_id().to_string();
        env.new_string(id).unwrap()
    } else {
        env.new_string("").unwrap()
    }
}

/*
/// JNI: IrohBridge.sendMessage(data: byte[]): Boolean
#[no_mangle]
pub extern "system" fn Java_nz_scuttlebutt_tremolavossbol_tssb_iroh_IrohBridge_sendMessage(
    env: JNIEnv,
    _class: JClass,
    msg_bytes: jbyteArray,
) -> jboolean {
    // convert Java byte[] → Vec<u8>
    let buf = match env
        .convert_byte_array(unsafe { JByteArray::from_raw(msg_bytes) })
    {
        Ok(b) => b,
        Err(e) => {
            log::error!("sendMessage: bad byte array: {:?}", e);
            return 0;
        }
    };

    // grab the stored SendStream
    let lock = match SEND_STREAM.get() {
        Some(m) => m,
        None => return 0,
    };

    // fire & forget the write, but **Base64‑encode** the payload
    let rt = RT.get().unwrap();
    // let encoded = STANDARD.encode(&buf);
    rt.spawn(async move {
        let mut tx = lock.lock().await;
        if let Err(e) = tx.write_all(&buf).await {
            log::error!("sendMessage write_all raw: {:?}", e);
        }
    });

    1
}
*/

/// JNI: IrohBridge.registerListener(listener: IrohMessageListener): Boolean
#[no_mangle]
pub extern "system" fn Java_nz_scuttlebutt_tremolavossbol_tssb_iroh_IrohBridge_registerListener(
    mut env: JNIEnv,
    _class: JClass,
    listener: JObject,
) -> jboolean {
    if let Ok(vm) = env.get_java_vm() {
        android_glue::JVM.set(vm).ok();
    } else {
        return 0;
    }
    if let Ok(gr) = env.new_global_ref(listener) {
        android_glue::JAVA_LISTENER.set(gr).ok();
        1
    } else {
        0
    }
}


#[no_mangle]
pub extern "system" fn Java_nz_scuttlebutt_tremolavossbol_tssb_iroh_IrohBridge_sendPacket(
    env: JNIEnv,
    _class: JClass,
    data: jbyteArray,
) -> jboolean {
    let buf = match env.convert_byte_array(unsafe { JByteArray::from_raw(data) }) {
        Ok(b) => b,
        Err(_) => return 0,
    };
    if let Some(node) = NODE.get() {
        let node = node.clone();
        let rt = RT.get().unwrap();
        rt.spawn(async move {
            if let Err(e) = node.send_packet(&buf).await {
                log::error!("send_packet failed: {:?}", e);
            }
        });
        1
    } else {
        0
    }
}

// eof
