// ===== ETAPA 3: ENVIO DOS DADOS PARA O BACKEND =====
// Cole este bloco no seu código existente (includes e variáveis no topo,
// funções antes do setup(), e as chamadas indicadas dentro de setup() e loop()).

#include <WiFi.h>
#include <HTTPClient.h>
#include <time.h>

// --- Configuração (ajuste) ---
const char* WIFI_SSID  = "SUA_REDE";            // no Wokwi: "Wokwi-GUEST"
const char* WIFI_PASS  = "SUA_SENHA";           // no Wokwi: ""
const char* SERVER_URL = "http://192.168.0.10:8000/leituras"; // IP do PC que roda o Flask

// --- Buffer para reenvio quando falhar ---
const int BUF_MAX = 30;
String bufferEnvio[BUF_MAX];
int bufCount = 0;

unsigned long ultimoEnvio = 0;
const unsigned long INTERVALO_ENVIO = 2000; // 2 segundos

void conectarWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("Conectando ao WiFi");
  int tentativas = 0;
  while (WiFi.status() != WL_CONNECTED && tentativas < 30) {
    delay(500);
    Serial.print(".");
    tentativas++;
  }
  Serial.println(WiFi.status() == WL_CONNECTED ? "\nWiFi conectado!" : "\nFalha no WiFi");
  // Hora em UTC via NTP
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
}

String timestampISO() {
  struct tm t;
  if (!getLocalTime(&t, 100)) return "1970-01-01T00:00:00Z";
  char b[25];
  strftime(b, sizeof(b), "%Y-%m-%dT%H:%M:%SZ", &t);
  return String(b);
}

bool enviarJson(const String& json) {
  if (WiFi.status() != WL_CONNECTED) return false;
  HTTPClient http;
  http.begin(SERVER_URL);
  http.addHeader("Content-Type", "application/json");
  int codigo = http.POST(json);
  http.end();
  Serial.printf("HTTP POST -> codigo %d\n", codigo);
  return (codigo == 200 || codigo == 201);
}

void guardarNoBuffer(const String& json) {
  if (bufCount >= BUF_MAX) {            // buffer cheio: descarta o mais antigo
    for (int i = 1; i < BUF_MAX; i++) bufferEnvio[i - 1] = bufferEnvio[i];
    bufCount--;
  }
  bufferEnvio[bufCount++] = json;
  Serial.printf("Falha no envio. Leituras no buffer: %d\n", bufCount);
}

void reenviarBuffer() {
  while (bufCount > 0) {
    if (!enviarJson(bufferEnvio[0])) return;   // para no primeiro erro
    for (int i = 1; i < bufCount; i++) bufferEnvio[i - 1] = bufferEnvio[i];
    bufCount--;
    Serial.printf("Reenviado do buffer. Restam: %d\n", bufCount);
  }
}

// Chame esta função a cada 2 segundos com os valores já calculados
void registrarLeitura(float tempC, float umidPct, int luz, int presenca, float probVida) {
  char corpo[256];
  snprintf(corpo, sizeof(corpo),
    "{\"timestamp\":\"%s\",\"temperatura_c\":%.1f,\"umidade_pct\":%.1f,"
    "\"luminosidade\":%d,\"presenca\":%d,\"probabilidade_vida\":%.1f}",
    timestampISO().c_str(), tempC, umidPct, luz, presenca, probVida);
  String json(corpo);

  if (WiFi.status() != WL_CONNECTED) WiFi.reconnect();

  reenviarBuffer();                 // primeiro tenta esvaziar o que ficou pendente
  if (!enviarJson(json)) guardarNoBuffer(json);
}

/* ===== COMO INTEGRAR =====

void setup() {
  Serial.begin(115200);
  // ... seu setup atual ...
  conectarWiFi();                  // <- adicione
}

void loop() {
  // ... leitura dos sensores e cálculo da probabilidade (já existentes) ...

  if (millis() - ultimoEnvio >= INTERVALO_ENVIO) {
    ultimoEnvio = millis();
    // use os nomes das SUAS variáveis:
    registrarLeitura(temperatura, umidade, luminosidade, presenca, probabilidadeVida);
  }
}
*/
