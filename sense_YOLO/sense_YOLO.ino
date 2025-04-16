#include "esp_camera.h"
#include <WiFi.h>
#include "esp_http_server.h"

// WiFi Credentials
const char* ssid = "JioFiber-rR7qD";
const char* password = "dohToukei3ooChie";

// Camera Pin Definitions (ESP32-S3 Sense)
#define PWDN_GPIO_NUM     -1
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM     10
#define SIOD_GPIO_NUM     40
#define SIOC_GPIO_NUM     39
#define Y9_GPIO_NUM       48
#define Y8_GPIO_NUM       11
#define Y7_GPIO_NUM       12
#define Y6_GPIO_NUM       14
#define Y5_GPIO_NUM       16
#define Y4_GPIO_NUM       18
#define Y3_GPIO_NUM       17
#define Y2_GPIO_NUM       15
#define VSYNC_GPIO_NUM    38
#define HREF_GPIO_NUM     47
#define PCLK_GPIO_NUM     13

httpd_handle_t camera_httpd = NULL;
camera_config_t config;

// === Optional GPIO to trigger on detection ===
#define DETECTED_GPIO 2

// --- Set Dynamic Resolution ---
void setResolution(framesize_t size) {
  config.frame_size = size;
  esp_camera_deinit();
  esp_camera_init(&config);
}

// --- Camera JPEG Handlers ---
esp_err_t jpg_handler(httpd_req_t *req, framesize_t res) {
  setResolution(res);
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    httpd_resp_send_500(req);
    return ESP_FAIL;
  }
  httpd_resp_set_type(req, "image/jpeg");
  httpd_resp_send(req, (const char *)fb->buf, fb->len);
  esp_camera_fb_return(fb);
  return ESP_OK;
}

esp_err_t jpg_lo_handler(httpd_req_t *req) {
  return jpg_handler(req, FRAMESIZE_QQVGA);
}
esp_err_t jpg_mid_handler(httpd_req_t *req) {
  return jpg_handler(req, FRAMESIZE_QVGA);
}
esp_err_t jpg_hi_handler(httpd_req_t *req) {
  return jpg_handler(req, FRAMESIZE_VGA);
}

// --- Handle POST Detection Data ---
esp_err_t detect_post_handler(httpd_req_t *req) {
  char content[100];
  size_t recv_len = httpd_req_recv(req, content, sizeof(content) - 1);
  if (recv_len <= 0) return ESP_FAIL;

  content[recv_len] = '\0';
  Serial.print("Detected Object: ");
  Serial.println(content);

  // Example: trigger GPIO if person detected
  if (strstr(content, "PERSON") != NULL) {
    digitalWrite(DETECTED_GPIO, HIGH);
    delay(2000);
    digitalWrite(DETECTED_GPIO, LOW);
  }

  httpd_resp_send(req, "Received", HTTPD_RESP_USE_STRLEN);
  return ESP_OK;
}

void startCameraServer() {
  httpd_config_t config_server = HTTPD_DEFAULT_CONFIG();
  if (httpd_start(&camera_httpd, &config_server) == ESP_OK) {
    // Camera routes
    httpd_uri_t uri_lo = {"/cam-lo.jpg", HTTP_GET, jpg_lo_handler, NULL};
    httpd_uri_t uri_mid = {"/cam-mid.jpg", HTTP_GET, jpg_mid_handler, NULL};
    httpd_uri_t uri_hi = {"/cam-hi.jpg", HTTP_GET, jpg_hi_handler, NULL};
    httpd_register_uri_handler(camera_httpd, &uri_lo);
    httpd_register_uri_handler(camera_httpd, &uri_mid);
    httpd_register_uri_handler(camera_httpd, &uri_hi);

    // Detection POST route
    httpd_uri_t detect_uri = {
      .uri       = "/detect",
      .method    = HTTP_POST,
      .handler   = detect_post_handler,
      .user_ctx  = NULL
    };
    httpd_register_uri_handler(camera_httpd, &detect_uri);

    Serial.println("Camera endpoints:");
    Serial.println("  /cam-lo.jpg");
    Serial.println("  /cam-mid.jpg");
    Serial.println("  /cam-hi.jpg");
    Serial.println("  POST to /detect");
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(DETECTED_GPIO, OUTPUT);
  digitalWrite(DETECTED_GPIO, LOW);

  // Camera init
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 10000000;
  config.pixel_format = PIXFORMAT_JPEG;

  if (psramFound()) {
    config.frame_size = FRAMESIZE_VGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_QVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  if (esp_camera_init(&config) != ESP_OK) {
    Serial.println("Camera init failed!");
    return;
  }

  // WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected.");
  Serial.print("Visit: http://");
  Serial.println(WiFi.localIP());

  startCameraServer();
}

void loop() {}
