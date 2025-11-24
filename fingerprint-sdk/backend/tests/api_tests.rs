use axum::{
    body::{Body, to_bytes},
    http::{Request, StatusCode},
};
use tower::ServiceExt;
use serde_json::{json, Value};
use base64::Engine;

mod common;

use common::*;

#[tokio::test]
async fn test_store_template() {
    let app = create_test_app().await;
    let test_template = base64::engine::general_purpose::STANDARD.encode("test_template_data");

    let response = app
        .oneshot(
            Request::builder()
                .method("POST")
                .uri("/users/123/fingerprint")
                .header("content-type", "application/json")
                .body(Body::from(
                    serde_json::to_string(&json!({
                        "template": test_template,
                        "finger": "LEFT_INDEX"
                    }))
                    .unwrap(),
                ))
                .unwrap(),
        )
        .await
        .unwrap();

    assert_eq!(response.status(), StatusCode::OK);

    let body = to_bytes(response.into_body(), usize::MAX).await.unwrap();
    let json: Value = serde_json::from_slice(&body).unwrap();
    assert_eq!(json["success"], true);
}

#[tokio::test]
async fn test_load_template() {
    let test_template = base64::engine::general_purpose::STANDARD.encode("test_template_data");

    // First store a template
    let app1 = create_test_app().await;
    let _ = app1
        .oneshot(
            Request::builder()
                .method("POST")
                .uri("/users/123/fingerprint")
                .header("content-type", "application/json")
                .body(Body::from(
                    serde_json::to_string(&json!({
                        "template": test_template,
                        "finger": "LEFT_INDEX"
                    }))
                    .unwrap(),
                ))
                .unwrap(),
        )
        .await
        .unwrap();

    // Then load it
    let app2 = create_test_app().await;
    let response = app2
        .oneshot(
            Request::builder()
                .method("GET")
                .uri("/users/123/fingerprint")
                .body(Body::empty())
                .unwrap(),
        )
        .await
        .unwrap();

    assert_eq!(response.status(), StatusCode::OK);

    let body = to_bytes(response.into_body(), usize::MAX).await.unwrap();
    let json: Value = serde_json::from_slice(&body).unwrap();
    assert_eq!(json["template"], test_template);
    assert_eq!(json["finger"], "LEFT_INDEX");
}

#[tokio::test]
async fn test_load_template_not_found() {
    let app = create_test_app().await;

    let response = app
        .oneshot(
            Request::builder()
                .method("GET")
                .uri("/users/999/fingerprint")
                .body(Body::empty())
                .unwrap(),
        )
        .await
        .unwrap();

    assert_eq!(response.status(), StatusCode::NOT_FOUND);
}

#[tokio::test]
async fn test_load_templates() {
    let test_template1 = base64::engine::general_purpose::STANDARD.encode("test_template_1");
    let test_template2 = base64::engine::general_purpose::STANDARD.encode("test_template_2");

    // Store first template
    let app1 = create_test_app().await;
    let _ = app1
        .oneshot(
            Request::builder()
                .method("POST")
                .uri("/users/123/fingerprint")
                .header("content-type", "application/json")
                .body(Body::from(
                    serde_json::to_string(&json!({
                        "template": test_template1,
                        "finger": "LEFT_INDEX"
                    }))
                    .unwrap(),
                ))
                .unwrap(),
        )
        .await
        .unwrap();

    // Store second template
    let app2 = create_test_app().await;
    let _ = app2
        .oneshot(
            Request::builder()
                .method("POST")
                .uri("/users/456/fingerprint")
                .header("content-type", "application/json")
                .body(Body::from(
                    serde_json::to_string(&json!({
                        "template": test_template2,
                        "finger": "RIGHT_INDEX"
                    }))
                    .unwrap(),
                ))
                .unwrap(),
        )
        .await
        .unwrap();

    // Load all templates
    let app3 = create_test_app().await;
    let response = app3
        .oneshot(
            Request::builder()
                .method("GET")
                .uri("/templates")
                .body(Body::empty())
                .unwrap(),
        )
        .await
        .unwrap();

    assert_eq!(response.status(), StatusCode::OK);

    let body = to_bytes(response.into_body(), usize::MAX).await.unwrap();
    let json: Value = serde_json::from_slice(&body).unwrap();
    assert!(json.is_array());
    assert!(json.as_array().unwrap().len() >= 2);
}

#[tokio::test]
async fn test_log_auth() {
    let app = create_test_app().await;

    let response = app
        .oneshot(
            Request::builder()
                .method("POST")
                .uri("/log_auth")
                .header("content-type", "application/json")
                .body(Body::from(
                    serde_json::to_string(&json!({
                        "user_id": 123,
                        "success": true,
                        "score": 0.85
                    }))
                    .unwrap(),
                ))
                .unwrap(),
        )
        .await
        .unwrap();

    assert_eq!(response.status(), StatusCode::OK);

    let body = to_bytes(response.into_body(), usize::MAX).await.unwrap();
    let json: Value = serde_json::from_slice(&body).unwrap();
    assert_eq!(json["success"], true);
}

#[tokio::test]
async fn test_store_template_invalid_request() {
    let app = create_test_app().await;

    let response = app
        .oneshot(
            Request::builder()
                .method("POST")
                .uri("/users/123/fingerprint")
                .header("content-type", "application/json")
                .body(Body::from(
                    serde_json::to_string(&json!({
                        "template": "",
                        "finger": "LEFT_INDEX"
                    }))
                    .unwrap(),
                ))
                .unwrap(),
        )
        .await
        .unwrap();

    assert_eq!(response.status(), StatusCode::BAD_REQUEST);
}

