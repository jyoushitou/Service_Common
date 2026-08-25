//引入异步读写trait
use tokio::io::{AsyncReadExt, AsyncWriteExt};
//配合上面的trait
use tokio::net::TcpStream;

use crate::Utils;

// 收到的数据结构
const HEAD_ID_LENGTH: usize = 8;
const HEAD_LEN_LENGTH: usize = 4;
pub const HEAD_LENGTH: usize = HEAD_ID_LENGTH + HEAD_LEN_LENGTH;
pub const MAX_LENGTH: usize = 1024 * 1024;

// 读一帧(即读数据)
pub async fn read_frame(stream: &mut TcpStream) -> std::io::Result<(u64, Vec<u8>)> {
    Utils::out::out_msg("正在准备读数据");
    // 初始化数组
    let mut head = [0u8; HEAD_LENGTH];

    // 读数据
    stream.read_exact(&mut head).await?;

    // 读头节点
    let msg_id = u64::from_be_bytes(head[0..HEAD_ID_LENGTH].try_into().unwrap());
    let msg_len =
        u32::from_be_bytes(head[HEAD_ID_LENGTH..HEAD_LENGTH].try_into().unwrap()) as usize;

    Utils::out::out_msg("获取到头部部分数据，正在判断消息合法性");
    // 判断数据合法性
    if msg_len > MAX_LENGTH || msg_len <= 0 {
        return Err(std::io::Error::new(
            std::io::ErrorKind::InvalidData,
            format!("消息长度错了喵！，{}", msg_len),
        ));
    }

    Utils::out::out_net_msg(msg_id, &format!("收到的消息：msg_len={}", msg_len,));

    Utils::out::out_msg("头部校验通过，准备读取消息体数据");

    // 缓冲区
    let mut body = vec![0u8; msg_len];

    // 读取消息体
    stream.read_exact(&mut body).await?;

    Utils::out::out_msg(&format!("消息读取完毕，交给解析，msg_len={}", msg_len));

    //返回元组
    Ok((msg_id, body))
}

//写一帧
pub async fn write_frame(stream: &mut TcpStream, msg_id: u64, body: &[u8]) -> std::io::Result<()> {
    Utils::out::out_msg("准备发送数据");
    //创建缓冲区
    let mut buf = Vec::with_capacity(HEAD_LENGTH + body.len());

    Utils::out::out_msg("构建发送体中");
    //将数据都写入
    buf.extend_from_slice(&msg_id.to_be_bytes());
    buf.extend_from_slice(&(body.len() as u32).to_be_bytes());
    buf.extend_from_slice(body);

    Utils::out::out_msg("正在发送数据");
    //发送
    stream.write_all(&buf).await
}
