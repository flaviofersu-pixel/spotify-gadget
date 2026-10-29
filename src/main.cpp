#include <Arduino.h>

// Mini mando de Spotify - prototipo en Wokwi
// Mismas conexiones que en tu plan de montaje:
//   Pantalla Sharp: VIN->3V3, GND->GND, CLK->D8, DI->D10, CS->D3
//   Botones: D0 = anterior, D1 = play/pausa, D2 = siguiente (la otra patita a GND)
// Las canciones son de mentira: el Bluetooth vendrá en la fase 4.

#include <Adafruit_GFX.h>
#include <Adafruit_SharpMem.h>

#ifndef IRAM_ATTR
#define IRAM_ATTR  // en la XIAO nRF52840 no hace falta
#endif

#define NEGRO 0
#define BLANCO 1

// La misma línea que en la fase 3.5 del plan
Adafruit_SharpMem pantalla(D8, D10, D3, 144, 168);

const int PIN_ANTERIOR = D0;
const int PIN_PLAY = D1;
const int PIN_SIGUIENTE = D2;

struct Cancion {
  const char* titulo;
  const char* artista;
  int duracion;  // en segundos
};

Cancion lista[] = {
  { "Blinding Lights", "The Weeknd", 200 },
  { "Despacito", "Luis Fonsi", 229 },
  { "Shape of You", "Ed Sheeran", 233 },
  { "Flowers", "Miley Cyrus", 200 },
  { "Tusa", "Karol G", 201 },
};
const int TOTAL = sizeof(lista) / sizeof(lista[0]);

int actual = 0;          // número de la canción que suena
bool sonando = true;     // true = play, false = pausa
int segundo = 0;         // por dónde va la canción
unsigned long ultimoTic = 0;

// Los botones avisan solos (interrupción) en cuanto se pulsan,
// así no se pierde ningún clic aunque la pantalla esté dibujando.
const int pines[3] = { PIN_ANTERIOR, PIN_PLAY, PIN_SIGUIENTE };
volatile bool pulsado[3] = { false, false, false };
volatile unsigned long ultimaPulsacion[3] = { 0, 0, 0 };

void IRAM_ATTR marcar(int i) {
  unsigned long ahora = millis();
  if (ahora - ultimaPulsacion[i] > 150) {  // ignora rebotes
    pulsado[i] = true;
    ultimaPulsacion[i] = ahora;
  }
}
void IRAM_ATTR alPulsarAnterior() { marcar(0); }
void IRAM_ATTR alPulsarPlay() { marcar(1); }
void IRAM_ATTR alPulsarSiguiente() { marcar(2); }

bool sePulso(int i) {
  if (pulsado[i]) {
    pulsado[i] = false;
    return true;
  }
  return false;
}

void escribirTiempo(int x, int y, int s) {
  pantalla.setCursor(x, y);
  pantalla.print(s / 60);
  pantalla.print(':');
  if (s % 60 < 10) pantalla.print('0');
  pantalla.print(s % 60);
}

// Escribe un texto centrado; si es muy largo, usa letra pequeña
void textoCentrado(const char* texto, int y, int tamano) {
  if (strlen(texto) * 6 * tamano > 144) tamano = 1;
  pantalla.setTextSize(tamano);
  int ancho = strlen(texto) * 6 * tamano;
  pantalla.setCursor(max(0, (144 - ancho) / 2), y);
  pantalla.print(texto);
}

void dibujarDisco(int cx, int cy) {
  pantalla.fillCircle(cx, cy, 26, NEGRO);
  pantalla.drawCircle(cx, cy, 20, BLANCO);
  pantalla.drawCircle(cx, cy, 14, BLANCO);
  pantalla.fillCircle(cx, cy, 6, BLANCO);
  pantalla.fillCircle(cx, cy, 2, NEGRO);
  // Una marca que gira mientras suena la canción
  float angulo = segundo * 0.8;
  int mx = cx + cos(angulo) * 23;
  int my = cy + sin(angulo) * 23;
  pantalla.fillCircle(mx, my, 2, BLANCO);
}

void dibujarBotones(int y) {
  // Anterior |<<
  pantalla.fillRect(22, y - 6, 3, 13, NEGRO);
  pantalla.fillTriangle(34, y - 6, 34, y + 6, 26, y, NEGRO);
  // Play o pausa, dentro de un círculo
  pantalla.drawCircle(72, y, 12, NEGRO);
  if (sonando) {
    pantalla.fillRect(66, y - 6, 4, 13, NEGRO);
    pantalla.fillRect(74, y - 6, 4, 13, NEGRO);
  } else {
    pantalla.fillTriangle(68, y - 7, 68, y + 7, 79, y, NEGRO);
  }
  // Siguiente >>|
  pantalla.fillTriangle(110, y - 6, 110, y + 6, 118, y, NEGRO);
  pantalla.fillRect(119, y - 6, 3, 13, NEGRO);
}

void dibujar() {
  Cancion& c = lista[actual];
  pantalla.clearDisplayBuffer();
  pantalla.setTextColor(NEGRO);
  pantalla.setTextWrap(false);

  // Barra de arriba (negra con letras blancas)
  pantalla.fillRect(0, 0, 144, 14, NEGRO);
  pantalla.setTextColor(BLANCO);
  pantalla.setTextSize(1);
  pantalla.setCursor(4, 3);
  pantalla.print("Spotify");
  pantalla.setCursor(98, 3);
  pantalla.print(actual + 1);
  pantalla.print(" de ");
  pantalla.print(TOTAL);
  pantalla.setTextColor(NEGRO);

  dibujarDisco(72, 46);

  textoCentrado(c.titulo, 80, 2);
  textoCentrado(c.artista, 99, 1);

  // Barra de progreso
  pantalla.drawRect(4, 113, 136, 7, NEGRO);
  int relleno = (long)134 * segundo / c.duracion;
  pantalla.fillRect(5, 114, relleno, 5, NEGRO);
  pantalla.setTextSize(1);
  escribirTiempo(4, 124, segundo);
  escribirTiempo(110, 124, c.duracion);

  dibujarBotones(150);

  pantalla.refresh();
}

void cambiarCancion(int paso) {
  actual = (actual + paso + TOTAL) % TOTAL;
  segundo = 0;
  Serial.print("Cancion: ");
  Serial.println(lista[actual].titulo);
}

void setup() {
  Serial.begin(115200);
  for (int i = 0; i < 3; i++) pinMode(pines[i], INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_ANTERIOR), alPulsarAnterior, FALLING);
  attachInterrupt(digitalPinToInterrupt(PIN_PLAY), alPulsarPlay, FALLING);
  attachInterrupt(digitalPinToInterrupt(PIN_SIGUIENTE), alPulsarSiguiente, FALLING);

  pantalla.begin();
  pantalla.clearDisplay();
  dibujar();
  Serial.println("Mando listo");
}

void loop() {
  bool cambio = false;

  if (sePulso(0)) {
    Serial.println("Boton ANTERIOR (D0)");
    // Como en Spotify: si ya lleva más de 3 s, vuelve al principio
    if (segundo > 3) segundo = 0;
    else cambiarCancion(-1);
    cambio = true;
  }
  if (sePulso(1)) {
    sonando = !sonando;
    Serial.println(sonando ? "Boton PLAY (D1): suena" : "Boton PLAY (D1): pausa");
    cambio = true;
  }
  if (sePulso(2)) {
    Serial.println("Boton SIGUIENTE (D2)");
    cambiarCancion(1);
    cambio = true;
  }

  // Cada segundo avanza la canción si está sonando
  if (millis() - ultimoTic >= 1000) {
    ultimoTic = millis();
    if (sonando) {
      segundo++;
      if (segundo >= lista[actual].duracion) cambiarCancion(1);
      cambio = true;
    }
  }

  if (cambio) dibujar();
  delay(10);
}
