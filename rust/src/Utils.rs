// Utils.rs

use std::sync::atomic::AtomicI32;

//服务器的ID
pub static SERVICE_ID: AtomicI32 = AtomicI32::new(13);

//初始化控制台
pub fn init() {}

//时间
pub mod time {
    use chrono::Local;

    //当前时间
    pub fn now_time() -> String {
        return Local::now().format("%Y-%m-%d %H:%M:%S").to_string();
    }

    //当前日期
    pub fn now_day() -> String {
        return Local::now().format("%Y-%m-%d").to_string();
    }
}

//输出
pub mod out {
    use super::SERVICE_ID;

    use super::file;
    use super::time;
    use crate::message::ServiceID;
    use std::sync::atomic::Ordering;

    //正常信息输出
    pub fn out_msg(msg: &str) {
        let msg = format!(
            "[{}][INFO]{} {}",
            ServiceID(SERVICE_ID.load(Ordering::Relaxed)),
            time::now_time(),
            msg
        );
        println!("{}", msg);
        file::out_log(&msg);
    }

    //错误的信息输出
    pub fn out_err(msg: &str) {
        let msg = format!(
            "[{}][ERROR]{} {}",
            ServiceID(SERVICE_ID.load(Ordering::Relaxed)),
            time::now_time(),
            msg
        );
        eprintln!("{}", msg);
        file::out_log(&msg);
    }

    //输出网络消息
    pub fn out_net_msg(msg_id: u64, msg: &str) {
        let msg = format!("[信息ID:{}]:{}", msg_id, msg);
        out_msg(&msg);
    }
}

//写入文件
pub mod file {
    use std::fs::OpenOptions;
    use std::io::Write;

    use super::time;

    //追加写入文件
    pub fn out_file_add(addr: &str, msg: &str) -> bool {
        match OpenOptions::new().create(true).append(true).open(addr) {
            Ok(mut file) => {
                if let Err(e) = writeln!(file, "{}", msg) {
                    return false;
                } else {
                    true
                }
            }
            Err(_) => false,
        }
    }

    //写入日志
    pub fn out_log(msg: &str) {
        let addr = time::now_day() + ".txt";
        if !out_file_add(&addr, &msg) {
            eprint!("写入失败");
        }
    }
}

pub mod exit {
    use std::sync::{
        atomic::{AtomicBool, Ordering},
        {Condvar, Mutex},
    };

    use super::out::out_msg;
    use tokio::sync::Notify;

    //退出信号
    static EXIT_FLAG: Mutex<bool> = Mutex::new(false);
    //退出的条件变量
    static EXIT_CONDVAR: Condvar = Condvar::new();
    //是否已经唤醒
    static EXIT_CALLED: AtomicBool = AtomicBool::new(false);

    // 异步退出通知（const 静态，不需要 once_cell）
    static NOTIFY: Notify = Notify::const_new();

    //查询是否已经调用
    pub fn get_exitflag() -> bool {
        return *EXIT_FLAG.lock().unwrap();
    }

    //等待阻塞信号
    pub fn wait_exit() {
        //获取退出信号的变量
        let mut flag = EXIT_FLAG.lock().unwrap();

        // 标准 Condvar 等待模式：用 while 而不是 if，
        while !*flag {
            out_msg("正在退出");

            flag = EXIT_CONDVAR.wait(flag).unwrap();
        }
    }

    //主动触发信号
    pub fn recv_exit() {
        //防止重复调用
        if EXIT_CALLED.swap(true, Ordering::SeqCst) {
            return;
        }

        //加锁
        let mut flag = EXIT_FLAG.lock().unwrap();

        //修改标志
        *flag = true;

        //释放锁+唤醒所有的wait_exit()
        drop(flag);
        EXIT_CONDVAR.notify_all();

        // 同时触发异步通知（唤醒 wait_exit_async）
        NOTIFY.notify_waiters();
    }

    // 触发异步退出通知（额外提供，也可以在需要的地方单独调用）
    pub fn notify_exit() {
        NOTIFY.notify_waiters();
    }

    // 异步等待退出信号（可直接 .await，不需要轮询）
    pub async fn wait_exit_async() {
        NOTIFY.notified().await;
    }
}
#[cfg(test)]
mod tests {
    //use super::*;
    //#[test]
}
