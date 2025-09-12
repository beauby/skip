use actix_web::{web, App, HttpResponse, HttpServer, get, delete, post, patch};
use std::ffi::c_void;
use tokio::sync::{mpsc, oneshot};

type AppData = mpsc::Sender<SkipServiceCmd>;

#[post("/streams/{resource}")]
async fn create_stream(data: web::Data<AppData>) -> HttpResponse {
    HttpResponse::Ok().finish()
}

#[delete("/streams/{uuid}")]
async fn destroy_stream(data: web::Data<AppData>) -> HttpResponse {
    HttpResponse::Ok().finish()
}

#[post("/snapshot/{resource}")]
async fn create_snapshot(data: web::Data<AppData>) -> HttpResponse {
    HttpResponse::Ok().finish()
}

#[post("/snapshot/{resource}/lookup")]
async fn create_snapshot_lookup(data: web::Data<AppData>) -> HttpResponse {
    HttpResponse::Ok().finish()
}

#[patch("/inputs/{collection}")]
async fn update_input(data: web::Data<AppData>) -> HttpResponse {
    HttpResponse::Ok().finish()
}

#[get("/healthcheck")]
async fn healthcheck() -> HttpResponse {
    HttpResponse::Ok().finish()
}

#[get("/streams/{uuid}")]
async fn read_stream(data: web::Data<AppData>) -> HttpResponse {
    HttpResponse::Ok().finish()
}

fn run_control_server(service_tx: mpsc::Sender<SkipServiceCmd>, port: u32) -> std::io::Result<actix_web::dev::Server> {
    Ok(HttpServer::new(move || {
        App::new()
            .app_data(web::Data::new(service_tx.clone()))
            .service(
                web::scope("/v1")
                    .service(create_stream)
                    .service(destroy_stream)
                    .service(create_snapshot)
                    .service(create_snapshot_lookup)
                    .service(update_input)
                    .service(healthcheck)
            )
    }).bind(format!("0.0.0.0:{}", port))?
        .run())
}

fn run_streaming_server(service_tx: mpsc::Sender<SkipServiceCmd>, port: u32) -> std::io::Result<actix_web::dev::Server> {
    Ok(HttpServer::new(move || {
        App::new()
            .app_data(web::Data::new(service_tx.clone()))
            .service(
            web::scope("/v1")
                .service(read_stream)
            )
    }).bind(format!("0.0.0.0:{}", port))?
       .run())
}

async fn run_servers(
    service_tx: mpsc::Sender<SkipServiceCmd>,
    control_port: u32,
    streaming_port: u32,
    join_tx: mpsc::Sender<()>,
    mut cancel_rx: mpsc::Receiver<()>
) {
    let control_server = run_control_server(service_tx.clone(), control_port).unwrap();
    let control_server_handle = control_server.handle();
    let streaming_server = run_streaming_server(service_tx, streaming_port).unwrap();
    let streaming_server_handle = streaming_server.handle();
    tokio::select! {
        _ = control_server => {
            streaming_server_handle.stop(true).await
        }
        _ = streaming_server => {
            control_server_handle.stop(true).await
        }
        Some(_) = cancel_rx.recv() => {
            println!("Cancellation signal received!");
            control_server_handle.stop(true).await;
            streaming_server_handle.stop(true).await;
        }
    }
    _ = join_tx.send(()).await
}

struct ServiceHandle {
    cancel_tx: mpsc::Sender<()>,
    join_rx: mpsc::Receiver<()>,
    cancel_skip_service_tx: mpsc::Sender<()>,
}

enum SkipServiceCmd {
    CreateStream(),
    DestroyStream(),
    SubscribeStream(),
    UnsubscribeStream(),
    CreateSnapshot(),
    CreateSnapshotLookup(),
    UpdateInput(),
}

struct ForeignPtr(*mut c_void);
unsafe impl Send for ForeignPtr {}

struct SkipService {
    ptr: ForeignPtr,
    cmd_rx: mpsc::Receiver<SkipServiceCmd>,
}

impl SkipService {
    fn new(service_ptr: *mut c_void, cmd_rx: mpsc::Receiver<SkipServiceCmd>) -> Self {
        // let (tx, cmd_rx) = mpsc::channel(1024);
        SkipService{ptr: ForeignPtr(service_ptr), cmd_rx}
    }
    
    async fn run(mut self, cancel_rx: mpsc::Receiver<()>) {
        loop {
            if let Some(cmd) = self.cmd_rx.recv().await {
                match cmd {
                    _ => todo!()
                }
            } else {
                break
            }
        }
    }
}

pub extern "C" fn start_service(service_ptr: *mut c_void, control_port: u32, streaming_port: u32) -> *mut c_void {
    let (cancel_skip_service_tx, cancel_skip_service_rx) = mpsc::channel(1);
    let (cmd_tx, cmd_rx) = mpsc::channel(1024);
    let skip_service = SkipService::new(service_ptr, cmd_rx);
    std::thread::spawn(move || {
        let rt = actix_rt::System::new();
        rt.block_on(skip_service.run(cancel_skip_service_rx))
    });
    let (cancel_tx, cancel_rx) = mpsc::channel(1);
    let (join_tx, join_rx) = mpsc::channel(1);
    std::thread::spawn(move || {
        let rt = actix_rt::System::new();
        rt.block_on(
            run_servers(cmd_tx, control_port, streaming_port, join_tx, cancel_rx)
        );
    });
    let handle = Box::new(ServiceHandle {cancel_tx, join_rx, cancel_skip_service_tx});

    Box::into_raw(handle) as *mut c_void
}

pub extern "C" fn stop_service(handle_ptr: *mut c_void) {
    let handle = unsafe { &mut *(handle_ptr as *mut ServiceHandle) };
    std::thread::spawn(move || async {
        _ = handle.cancel_tx.send(()).await;
        _ = handle.join_rx.recv().await;
        handle.cancel_skip_service_tx.send(()).await
    });
}

pub extern "C" fn join_service(handle_ptr: *mut c_void) {
    let mut handle = unsafe { Box::from_raw(handle_ptr as *mut ServiceHandle) };
    let rt = actix_rt::System::new();
    rt.block_on(async {
        _ = handle.join_rx.recv().await;
        _ = handle.cancel_skip_service_tx.send(()).await
    })
}
