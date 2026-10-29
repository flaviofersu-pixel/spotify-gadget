# Mini mando en VS Code + Wokwi

## Una sola vez: preparar VS Code

1. En VS Code, ve a **Extensiones** (el icono de los cuadraditos, a la izquierda).
2. Instala **PlatformIO IDE**. Tarda unos minutos; cuando acabe, reinicia VS Code si te lo pide.
3. Instala **Wokwi Simulator**.
4. Pulsa **F1**, escribe `Wokwi: Request a New License` y sigue los pasos en el navegador (es gratis). Esto activa el simulador.

## Abrir el proyecto

1. Descomprime `mini-mando-vscode.zip` en una carpeta, por ejemplo en Documentos.
2. En VS Code: **File → Open Folder…** y elige la carpeta `mini-mando-vscode` (la que tiene dentro `platformio.ini`).
3. Espera a que PlatformIO termine de preparar el proyecto (abajo a la derecha lo indica). La primera vez descarga cosas y tarda unos minutos.

## Compilar y simular

1. **Compilar:** pulsa el ✓ de la barra azul de abajo (PlatformIO: Build). Espera a que ponga **SUCCESS**.
2. **Simular:** abre el archivo `diagram.json`. Se abre el dibujo. Pulsa **▶** encima del dibujo (o **F1** → `Wokwi: Start Simulator`).
3. Haz clic en los botones del dibujo: **Anterior (D0)**, **Play (D1)** y **Siguiente (D2)**. También vale el teclado: flecha izquierda, espacio, flecha derecha.

Cada vez que cambies el programa (`src/main.cpp`), vuelve a pulsar ✓ y después reinicia la simulación.

## Qué es cada archivo

| Archivo | Para qué sirve |
|---|---|
| `src/main.cpp` | El programa del mando |
| `diagram.json` | El dibujo: piezas y cables |
| `platformio.ini` | Qué placa y qué librerías usa |
| `wokwi.toml` | Le dice a Wokwi dónde está el programa compilado y la pantalla |
| `chips/` | La imitación de tu pantalla Sharp (ya compilada, no hay que tocarla) |

## Los cables

| Color | Desde la XIAO | Hasta |
|---|---|---|
| rojo | 3V3 | pantalla VIN |
| negro | GND | pantalla GND |
| amarillo | D8 | pantalla CLK |
| verde | D10 | pantalla DI |
| azul | D3 | pantalla CS |
| naranja | D0 | botón Anterior |
| morado | D1 | botón Play |
| celeste | D2 | botón Siguiente |
| negros de abajo | GND | la otra patita de los 3 botones |

## Si algo falla

- **Wokwi dice que no encuentra `firmware.bin`:** primero tienes que compilar con el ✓.
- **Error al compilar:** copia el texto rojo del terminal y mándamelo.
- **La pantalla se queda gris:** mándame una captura de todo VS Code.
