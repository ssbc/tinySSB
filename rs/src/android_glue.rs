// src/android_glue.rs

use once_cell::sync::OnceCell;
use jni::objects::GlobalRef;
use jni::JavaVM;
use tokio::sync::{broadcast, Mutex as TokioMutex};

/// Stored Java VM so we can attach threads and call back into Kotlin
pub static JVM: OnceCell<JavaVM> = OnceCell::new();

/// Stored listener object (implements IrohMessageListener)
pub static JAVA_LISTENER: OnceCell<GlobalRef> = OnceCell::new();

use iroh::endpoint::Connection;

pub static SEND_STREAM: OnceCell<TokioMutex<iroh::endpoint::SendStream>> = OnceCell::new();

pub static SEND_CONN: OnceCell<TokioMutex<Connection>> = OnceCell::new();