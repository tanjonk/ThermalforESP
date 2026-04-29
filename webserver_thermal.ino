#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <Adafruit_MLX90640.h>

const char* ssid = "Barudak -4G";
const char* password = "Ngawi_123";

WebServer server(80);

// TFT
#define TFT_CS   15
#define TFT_DC   2
#define TFT_RST  4
#define TFT_LED  27

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);
Adafruit_MLX90640 mlx;

float frame[32*24];

int R_colour,G_colour,B_colour;

float T_min,T_max;

int i,j;



// =============================
// COLOUR MAP (SAMA DENGAN TFT)
// =============================

void getColour(int j)
{

 if (j >= 0 && j < 30){
  R_colour = 0;
  G_colour = 0;
  B_colour = 20 + (120.0/30.0) * j;
 }

 else if (j < 60){
  R_colour = (120.0 / 30) * (j - 30.0);
  G_colour = 0;
  B_colour = 140 - (60.0/30.0) * (j - 30.0);
 }

 else if (j < 90){
  R_colour = 120 + (135.0/30.0) * (j - 60.0);
  G_colour = 0;
  B_colour = 80 - (70.0/30.0) * (j - 60.0);
 }

 else if (j < 120){
  R_colour = 255;
  G_colour = (60.0/30.0) * (j - 90.0);
  B_colour = 10 - (10.0/30.0) * (j - 90.0);
 }

 else if (j < 150){
  R_colour = 255;
  G_colour = 60 + (175.0/30.0) * (j - 120.0);
  B_colour = 0;
 }

 else{
  R_colour = 255;
  G_colour = 235 + (20.0/30.0) * (j - 150.0);
  B_colour = (255.0/30.0) * (j - 150.0);
 }

}



// =============================
// WEB PAGE
// =============================

void handleRoot(){

String html = R"====(
<html>
<h2>ESP32 Thermal Camera</h2>

<canvas id="thermal" width="384" height="288"></canvas>

<script>

function getColor(t){

 let r=0,g=0,b=0;

 if(t<0.2){
  r=40;
  g=0;
  b=120+600*t;
 }

 else if(t<0.4){
  r=0;
  g=100*(t-0.2)*5;
  b=255;
 }

 else if(t<0.6){
  r=255*(t-0.4)*5;
  g=0;
  b=255-(255*(t-0.4)*5);
 }

 else if(t<0.8){
  r=255;
  g=200*(t-0.6)*5;
  b=0;
 }

 else{
  r=255;
  g=255;
  b=255*(t-0.8)*5;
 }

 return "rgb("+Math.floor(r)+","+Math.floor(g)+","+Math.floor(b)+")";
}


async function update(){

 let res = await fetch('/thermal');
 let data = await res.json();

 let canvas = document.getElementById("thermal");
 let ctx = canvas.getContext("2d");

 let scale = 12;

 for(let y=0;y<24;y++){
  for(let x=0;x<32;x++){

    let temp=data[y][x];

    let t=(temp-20)/20;   // normalisasi suhu

    if(t<0) t=0;
    if(t>1) t=1;

    ctx.fillStyle=getColor(t);

    ctx.fillRect(x*scale,y*scale,scale,scale);

  }
 }

}

setInterval(update,200);

</script>
</html>
)====";

server.send(200,"text/html",html);
}
// =============================
// JSON THERMAL DATA
// =============================

void handleThermal(){

String json="[";

for(int y=0;y<24;y++){
  json+="[";
  for(int x=0;x<32;x++){

    json+=String(frame[y*32+x]);

    if(x<31) json+=",";
  }
  json+="]";
  if(y<23) json+=",";
}

json+="]";

server.send(200,"application/json",json);

}



// =============================
// SETUP
// =============================

void setup(){

Serial.begin(115200);

WiFi.begin(ssid,password);

while(WiFi.status()!=WL_CONNECTED){
 delay(500);
 Serial.print(".");
}

Serial.println("");
Serial.print("IP ESP32: ");
Serial.println(WiFi.localIP());


server.on("/",handleRoot);
server.on("/thermal",handleThermal);

server.begin();


Wire.begin(21,22);
Wire.setClock(400000);


// TFT

pinMode(TFT_LED, OUTPUT);
digitalWrite(TFT_LED, HIGH);

tft.begin();
tft.setRotation(1);
tft.fillScreen(ILI9341_BLACK);


// MLX

if (!mlx.begin(MLX90640_I2CADDR_DEFAULT, &Wire)) {
 Serial.println("MLX90640 not detected");
 while (1);
}

mlx.setMode(MLX90640_CHESS);
mlx.setResolution(MLX90640_ADC_18BIT);
mlx.setRefreshRate(MLX90640_4_HZ);

}



// =============================
// LOOP
// =============================

void loop(){

server.handleClient();


if (mlx.getFrame(frame) != 0){
 Serial.println("Frame error");
 return;
}


// hitung min max

T_min = frame[0];
T_max = frame[0];

for(i=1;i<768;i++){
 if(frame[i] < T_min) T_min = frame[i];
 if(frame[i] > T_max) T_max = frame[i];
}


// gambar ke TFT

for (i=0;i<24;i++){
 for(j=0;j<32;j++){

   float val = 180.0*(frame[i*32+j]-T_min)/(T_max-T_min);

   getColour(val);

   tft.fillRect(217 - j*7 , 35 + i*7 , 7 , 7 ,
   tft.color565(R_colour,G_colour,B_colour));

 }
}

}