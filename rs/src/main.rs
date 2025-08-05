mod chat_node;
mod android_glue;

use anyhow::{Context, Result};
use clap::{Arg, Command};
use tracing_subscriber::EnvFilter;
use iroh::NodeId;

use crate::chat_node::ChatNode;

#[tokio::main]
async fn main() -> Result<()> {
    tracing_subscriber::fmt()
        .with_env_filter(EnvFilter::new("cli=info"))
        .with_target(false)
        .with_level(false)
        .init();

    let matches = Command::new("Chat")
        .arg(
            Arg::new("start")
                .short('s')
                .long("start")
                .action(clap::ArgAction::SetTrue)
                .help("Start listening for incoming chats"),
        )
        .arg(
            Arg::new("connect")
                .short('c')
                .long("connect")
                .value_parser(clap::value_parser!(String))
                .help("Node ID to connect & chat with"),
        )
        .arg(
            Arg::new("node-id")
                .short('n')
                .long("node-id")
                .value_parser(clap::value_parser!(String))
                .required(true)
                .help("Hex‑encoded 32‑byte secret key for this node"),
        )
        .get_matches();

    let hex_secret = matches.get_one::<String>("node-id").unwrap();
    let node = ChatNode::spawn(hex_secret).await?;

    if matches.get_flag("start") {
        println!("Listening for incoming chats. Press Ctrl+C to stop.");
        tokio::signal::ctrl_c().await?;
    } else if let Some(peer_hex) = matches.get_one::<String>("connect") {
        let peer_id: NodeId = peer_hex.parse().context("Bad peer ID")?;
        node.connect_and_chat(peer_id).await?;
    } else {
        log::error!("Either --start or --connect <node-id> is required");
    }

    Ok(())
}
