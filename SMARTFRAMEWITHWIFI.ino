#include <SPI.h>
#include <SD.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h> 
#include <JPEGDEC.h>

// =====================================================
// PINS CONFIGURATION
// =====================================================
#define TFT_CS   5       // D1
#define TFT_DC   4       // D2
#define TFT_RST  -1      // 3.3V 
#define TFT_BLK  0       // D3 
#define SD_CS    15      // D8

const char* AP_SSID = "SMART-FRAME";
const char* AP_PASS = "12345678";
ESP8266WebServer server(80);

Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);
JPEGDEC jpeg;
File jpegFile;
File uploadFile;
bool sdOK = false;
bool renderNewPhoto = false; 

// =====================================================
// WIFI SETUP
// =====================================================
void startWiFi() {
  Serial.println("\nStarting WiFi...");
  WiFi.mode(WIFI_AP);
  delay(300);
  WiFi.softAP(AP_SSID, AP_PASS);
  Serial.println("WIFI READY");
  Serial.print("IP: "); Serial.println(WiFi.softAPIP());
}

// =====================================================
// SMART WEB-APP (Auto Resize & Brightness Slider)
// =====================================================
void handleRoot() {
  String html = R"=====(
  <!DOCTYPE html>
  <html>
  <head>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
      body { font-family: Arial; text-align: center; background: #111; color: white; padding: 20px; }
      .card { max-width: 400px; margin: auto; background: #222; padding: 25px; border-radius: 15px; box-shadow: 0 4px 8px rgba(0,0,0,0.5); }
      .btn { background: #2196F3; color: white; padding: 15px 30px; border-radius: 10px; border: none; font-size: 16px; cursor: pointer; width: 100%; margin-top: 15px; font-weight: bold; }
      .btn:active { background: #1976D2; }
      #preview { margin-top: 15px; width: 240px; height: 320px; border-radius: 10px; display: none; margin-left: auto; margin-right: auto; border: 2px solid #555; }
      #status { margin-top: 15px; color: #4CAF50; font-weight: bold; }
      .slider-container { margin-top: 30px; background: #333; padding: 15px; border-radius: 10px; }
      input[type=range] { width: 100%; margin-top: 10px; }
    </style>
  </head>
  <body>
    <div class="card">
      <h2>📸 Smart Photo Frame</h2>
      <p style="color:#aaa; font-size:14px;">Select any photo. It will auto-resize to fit the frame!</p>
      
      <input type="file" id="fileInput" accept="image/*" style="width:100%; padding: 10px 0;">
      <canvas id="canvas" width="240" height="320" style="display:none;"></canvas>
      <img id="preview" />
      
      <button class="btn" onclick="processImage()">UPLOAD PHOTO</button>
      <p id="status"></p>

      <!-- Brightness Control -->
      <div class="slider-container">
        <label>☀️ Brightness: <span id="bVal">100</span>%</label>
        <input type="range" min="0" max="100" value="100" id="brightSlider" oninput="updateBrightness(this.value)">
      </div>
    </div>

    <script>
      // Brightness Control Logic
      function updateBrightness(val) {
        document.getElementById('bVal').innerText = val;
        fetch('/brightness?val=' + val);
      }

      // Auto Resize Logic
      function processImage() {
        const file = document.getElementById('fileInput').files[0];
        if(!file) { alert('Please select a photo first!'); return; }
        
        document.getElementById('status').innerText = 'Resizing...';
        
        const reader = new FileReader();
        reader.onload = function(e) {
          const img = new Image();
          img.onload = function() {
            const canvas = document.getElementById('canvas');
            const ctx = canvas.getContext('2d');
            
            const targetRatio = 240 / 320;
            const imgRatio = img.width / img.height;
            let drawWidth, drawHeight, offsetX, offsetY;

            if (imgRatio > targetRatio) {
              drawHeight = 320;
              drawWidth = img.width * (320 / img.height);
              offsetX = (240 - drawWidth) / 2;
              offsetY = 0;
            } else {
              drawWidth = 240;
              drawHeight = img.height * (240 / img.width);
              offsetX = 0;
              offsetY = (320 - drawHeight) / 2;
            }

            ctx.fillStyle = "black";
            ctx.fillRect(0, 0, 240, 320);
            ctx.drawImage(img, offsetX, offsetY, drawWidth, drawHeight);

            document.getElementById('preview').src = canvas.toDataURL('image/jpeg');
            document.getElementById('preview').style.display = 'block';
            document.getElementById('status').innerText = 'Sending...';

            canvas.toBlob(function(blob) {
              const formData = new FormData();
              formData.append('photo', blob, 'photo.jpg');

              fetch('/upload', {
                method: 'POST',
                body: formData
              }).then(response => {
                document.getElementById('status').innerText = '✅ Success!';
              }).catch(error => {
                document.getElementById('status').innerText = '❌ Failed!';
              });
            }, 'image/jpeg', 1.0); 
          }
          img.src = e.target.result;
        }
        reader.readAsDataURL(file);
      }
    </script>
  </body>
  </html>
  )=====";
  
  server.send(200, "text/html", html);
}

// =====================================================
// BRIGHTNESS HANDLER
// =====================================================
void handleBrightness() {
  if (server.hasArg("val")) {
    int brightnessPercent = server.arg("val").toInt();
    // Convert 0-100% to 0-1023 (ESP8266 PWM range)
    int pwmValue = map(brightnessPercent, 0, 100, 0, 1023);
    analogWrite(TFT_BLK, pwmValue);
    server.send(200, "text/plain", "Brightness Updated");
  } else {
    server.send(400, "text/plain", "Missing Value");
  }
}

// =====================================================
// JPEG DECODER
// =====================================================
void* jpegOpen(const char* filename, int32_t* size) {
  jpegFile = SD.open(filename, FILE_READ);
  if (!jpegFile) { *size = 0; return NULL; }
  *size = jpegFile.size();
  return &jpegFile;
}

void jpegClose(void* handle) {
  if (jpegFile) jpegFile.close();
}

int32_t jpegRead(JPEGFILE* handle, uint8_t* buffer, int32_t length) {
  if (!jpegFile) return 0;
  return jpegFile.read(buffer, length);
}

int32_t jpegSeek(JPEGFILE* handle, int32_t position) {
  if (!jpegFile) return 0;
  return jpegFile.seek(position);
}

int JPEGDraw(JPEGDRAW* pDraw) {
  digitalWrite(SD_CS, HIGH); // Lock SD
  tft.drawRGBBitmap(pDraw->x, pDraw->y, pDraw->pPixels, pDraw->iWidth, pDraw->iHeight);
  return 1;
}

bool showJPG(const char* filename) {
  if (!SD.exists(filename)) return false;
  if (!jpeg.open(filename, jpegOpen, jpegClose, jpegRead, jpegSeek, JPEGDraw)) return false;

  int w = jpeg.getWidth();
  int h = jpeg.getHeight();
  int scale = 0;
  int dw = w, dh = h;
  
  int screen_w = 240;
  int screen_h = 320;

  if (w > screen_w * 4 || h > screen_h * 4) { scale = JPEG_SCALE_EIGHTH; dw = w/8; dh = h/8; }
  else if (w > screen_w * 2 || h > screen_h * 2) { scale = JPEG_SCALE_QUARTER; dw = w/4; dh = h/4; }
  else if (w > screen_w || h > screen_h) { scale = JPEG_SCALE_HALF; dw = w/2; dh = h/2; }

  int x = (screen_w - dw) / 2;
  int y = (screen_h - dh) / 2;
  if (x < 0) x = 0;
  if (y < 0) y = 0;

  tft.fillScreen(ST77XX_BLACK);
  jpeg.setPixelType(RGB565_LITTLE_ENDIAN);
  int result = jpeg.decode(x, y, scale);
  jpeg.close();

  return result ? true : false;
}

// =====================================================
// UPLOAD HANDLER
// =====================================================
void handleFileUpload() {
  HTTPUpload& upload = server.upload();

  if (upload.status == UPLOAD_FILE_START) {
    if (!sdOK) sdOK = SD.begin(SD_CS);
    if (SD.exists("/photo.jpg")) SD.remove("/photo.jpg");
    uploadFile = SD.open("/photo.jpg", FILE_WRITE);
  }
  else if (upload.status == UPLOAD_FILE_WRITE) {
    if (uploadFile) {
      uploadFile.write(upload.buf, upload.currentSize);
    }
  }
  else if (upload.status == UPLOAD_FILE_END) {
    if (uploadFile) uploadFile.close();
    renderNewPhoto = true; 
  }
  yield();
}

// =====================================================
// SETUP & LOOP
// =====================================================
void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(TFT_CS, OUTPUT);
  pinMode(SD_CS, OUTPUT);
  
  // Set BLK pin for Brightness (PWM)
  pinMode(TFT_BLK, OUTPUT);
  analogWrite(TFT_BLK, 1023); 
  
  SPI.begin();
  SPI.setFrequency(8000000); 
  
  startWiFi();
  sdOK = SD.begin(SD_CS);
  
  tft.init(240, 320); 
  tft.setRotation(90); 
  tft.invertDisplay(false); 
  
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);
  
  tft.setCursor(10, 40); tft.println("SMART FRAME");
  tft.setCursor(10, 80); tft.println("IP:");
  tft.setCursor(10, 110); tft.println(WiFi.softAPIP().toString());
  
  if(sdOK) {
    tft.setTextColor(ST77XX_GREEN);
    tft.setCursor(10, 160); tft.println("SD OK!");
  } else {
    tft.setTextColor(ST77XX_RED);
    tft.setCursor(10, 160); tft.println("SD FAIL!");
  }

  // Web Server Routes
  server.on("/", HTTP_GET, handleRoot);
  server.on("/brightness", HTTP_GET, handleBrightness); 
  server.on("/upload", HTTP_POST, [](){
    server.send(200, "text/plain", "OK"); 
  }, handleFileUpload);
  
  server.begin();
}

void loop() {
  server.handleClient();
  
  if (renderNewPhoto) {
    renderNewPhoto = false; 
    delay(500); 
    showJPG("/photo.jpg");
  }
  
  yield();
}
