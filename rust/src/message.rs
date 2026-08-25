//message.rs

use std::sync::atomic::Ordering;

use crate::Utils::SERVICE_ID;

//创建一个映射表
pub fn ServiceID(service_ID: i32) -> &'static str {
    match service_ID {
        1 => "RPCGateWay",
        13 => "Blog",
        _ => "Unknown",
    }
}
