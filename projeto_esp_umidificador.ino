#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <DHT.h>

// pino do relé
#define PINO_RELE 23  // conectado ao pino D23

// umidificador liga se a umidade cair abaixo desse valor
#define UMIDADE_MINIMA 45.0
//desliga o umidificador se a umidade subir acima desse valor
#define UMIDADE_MAXIMA 70.0

//wifi
const char* ssid = "";
const char* password = "";

//dht11
#define PINO_DHT 15           // pino gpio onde o sensor tá conectado
#define TIPO_DHT DHT11        // tipo do sensor
DHT dht(PINO_DHT, TIPO_DHT);  // objeto dht para interagir com o servidor


float temperatura = 0.0;  //inicia as variáveis
float umidade = 0.0;

// variaveis de tempo
unsigned long previousMillis = 0;
const long interval = 2000;
//intervalo de 2seg p/ ler o sensor


// umidificador ligado?
bool umidificadorLigado = false;

// cria objeto do servidor na porta 80
AsyncWebServer server(80);

// progmem pra armazenar o codigo html da pagina em memoria Flash
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="pt-br">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Monitoramento de umidade</title>
    <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
    <style>
        @import url("https://fonts.googleapis.com/css?family=Press+Start+2P");

        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Helvetica, Arial, sans-serif;
            display: flex;
            flex-direction: column;
            align-items: center;
            min-height: 100vh;
            background-color: #f0f8ff;
            color: #333;
            margin: 0;
            padding: 20px;
            box-sizing: border-box;
        }

        h1 {
            font-family: 'Press Start 2P', monospace;
            font-size: 24px;
            text-align: center;
            line-height: 1.5;
            color: #00008B;
            margin-bottom: 20px;
        }

        .container {
            max-width: 800px;
            text-align: center;
            padding: 20px;
            background-color: #ffffff;
            border-radius: 12px;
            box-shadow: 0 6px 12px rgba(0, 0, 0, 0.08);
            margin-bottom: 30px;
        }

        .container p {
            font-size: 14px;
            line-height: 1.6;
            text-align: justify;
            color: #555;
        }

        .capivara {
            display: block;
            margin: 25px auto 0 auto;
            width: 80px;
            image-rendering: pixelated;
        }

        .sensor-wrapper {
            display: flex;
            justify-content: center;
            gap: 30px;
            flex-wrap: wrap;
            width: 100%;
            max-width: 800px;
            margin-bottom: 30px;
        }

        .sensor-data {
            padding: 25px;
            border: none;
            border-radius: 12px;
            background-color: #ffffff;
            box-shadow: 0 6px 12px rgba(0, 0, 0, 0.08);
            text-align: center;
            width: 200px;
            transition: transform 0.2s ease;
        }
        
        .sensor-data:hover {
            transform: translateY(-5px);
        }

        .sensor-data h2 {
            font-size: 16px;
            font-family: 'Press Start 2P', monospace;
            color: #00008B;
            margin: 0 0 10px 0;
        }

        .sensor-data .value {
            font-size: 28px;
            font-weight: bold;
            color: #333;
        }

        /* grafico */
        .chart-container {
            width: 100%;
            max-width: 800px;
            padding: 20px;
            background-color: #ffffff;
            border-radius: 12px;
            box-shadow: 0 6px 12px rgba(0, 0, 0, 0.08);
        }
    </style>
</head>
<body>

    <h1>
        SENSOR AUTOMÁTICO <br>
        DE UMIDADE AMBIENTE
    </h1>

    <div class="container">
        <p>
            Este sistema foi criado para ajudar pessoas que sofrem com problemas respiratórios,
            garantindo um ambiente mais confortável. Ele monitora a temperatura
            e a umidade do ar em tempo real e, quando detecta que a umidade está muito baixa,
            liga automaticamente um umidificador, ajudando a melhorar a qualidade do ar.
            Assim, você pode acompanhar as condições do ambiente e respirar melhor todos os dias!
        </p>
    </div>

    <div class="sensor-wrapper">
        <div class="sensor-data">
            <h2>Temperatura</h2>
            <p><span class="value" id="temperatura">--</span> &deg;C</p>
        </div>

        <div class="sensor-data">
            <h2>Umidade</h2>
            <p><span class="value" id="umidade">--</span> %</p>
        </div>

        <div class="sensor-data">
            <h2>Umidificador</h2>
            <p><span class="value" id="status">--</span></p>
        </div>
    </div>

    <div class="chart-container">
        <canvas id="humidity-chart"></canvas>
    </div>

    <script>
        let humidityChart; //variável pro gráfico
        document.addEventListener('DOMContentLoaded', function() {
            
            //inicia o grafico
            const ctx = document.getElementById('humidity-chart').getContext('2d');
            humidityChart = new Chart(ctx, {
                type: 'line',
                data: {
                    labels: [], //eixo X (tempo)
                    datasets: [{
                        label: 'Umidade (%)',
                        data: [], //eixo y (umidade)
                        borderColor: 'rgba(0, 0, 139, 1)',
                        backgroundColor: 'rgba(0, 0, 139, 0.1)',
                        borderWidth: 2,
                        fill: true,
                        tension: 0.3
                    }]
                },
                options: {
                    responsive: true,
                    animation: false,
                    scales: {
                        y: {
                            min: 0,
                            max: 100,
                            ticks: {
                                //% no eixo Y
                                callback: function(value) {
                                    return value + '%';
                                }
                            }
                        },
                        x: {
                            ticks: {
                                display: false //nao mostra tempo no eixo X
                            }
                        }
                    },
                    plugins: {
                        title: {
                        display: true,
                        text: 'Gráfico - Umidade',
                        padding: {
                            top: 10,
                            bottom: 10
                        },
                        font: {
                            size: 16,
                            family: "'Press Start 2P', monospace"
                        },
                        color: '#00008B'
                    },

                        legend: {
                            display: false //
                        }
                    }
                }
            });

            // busca dados e inicia o intervalo
            getData(); //busca dados na hora
            setInterval(getData, 5000); //busca dados a cada 5seg
        });

        async function getData() {
            try {
                //1- requisiçao pra /data
                const response = await fetch('/data');
                
                //2-converte para json
                const data = await response.json();

                //3- atualiza os cards
                document.getElementById("temperatura").innerHTML = data.temperatura.toFixed(1);
                document.getElementById("umidade").innerHTML = data.umidade.toFixed(1);

                //4- atualiza o card de status
                const statusElement = document.getElementById("status");
                if (data.status === true) {
                    statusElement.innerHTML = "Ligado";
                    statusElement.style.color = "#28a745";
                } else {
                    statusElement.innerHTML = "Desligado";
                    statusElement.style.color = "#dc3545";
                }

                //5- atualiza o grafico
                updateChart(data.umidade);

            } catch (error) {
                console.error("erro ao buscar dados:", error);
                
                document.getElementById("status").innerHTML = "Falha";
                document.getElementById("status").style.color = "#ffc107";
            }
        }

        //funcao para atualizar o grafico
        function updateChart(newHumidity) {
            //tempo
            const now = new Date();
            const timeLabel = now.toTimeString().split(' ')[0];

            //adiciona os novos dados
            humidityChart.data.labels.push(timeLabel);
            humidityChart.data.datasets[0].data.push(newHumidity);

            //tamanho do grafico
            //o shift ta  removendo o item mais antigo (primeiro do array)
            if (humidityChart.data.labels.length > 20) {
                humidityChart.data.labels.shift();
                humidityChart.data.datasets[0].data.shift();
            }

            //redesenha o grafico
            humidityChart.update();
        }
    </script>
</body>
</html>
)rawliteral";


// função executada uma vez
void setup() {
  Serial.begin(115200);
  dht.begin();  // inicia o sensor dht

  // inicia o pino do relé
  pinMode(PINO_RELE, OUTPUT);
  //relé começa desligado
  digitalWrite(PINO_RELE, LOW);

  // inicia conexão wifi
  WiFi.begin(ssid, password);
  Serial.print("Conectando ao wifi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nwifi conectado!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());  // imprime o ip do esp32, que será usado para acessar ele

  // rotas serv web
  // o que o servidor deve fazer quando receber uma requisição para a página principal
  server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
    // responde enviando o codigo html
    request->send_P(200, "text/html", index_html);  // send_p pois a string está na memoria progmem
  });

  //rota /data
  server.on("/data", HTTP_GET, [](AsyncWebServerRequest* request) {
    // 1 casa decimal
    String tempStr = String(temperatura, 1);
    String humStr = String(umidade, 1);

    //status do rele (converte o boleano para uma string json)
    String statusStr = umidificadorLigado ? "true" : "false";

    // string json com os dados
    String jsonResponse = "{\"temperatura\":" + tempStr + ",\"umidade\":" + humStr + ",\"status\":" + statusStr + "}";

    // resposta como application/json
    request->send(200, "application/json", jsonResponse);
  });

  server.begin();  // inicia o servidor web
}

void loop() {
  // pega o tempo atual
  unsigned long currentMillis = millis();

  // verifica se o tempo de 2 segundos ja passou
  if (currentMillis - previousMillis >= interval) {
    // Salva o tempo da ultima leitura
    previousMillis = currentMillis;

    // le os dados do sensor
    float newTemp = dht.readTemperature();
    float newHum = dht.readHumidity();

    // leitura deu certo?
    if (isnan(newTemp) || isnan(newHum)) {
      Serial.println("Falha ao ler o sensor");
    } else {
      // atualiza as variaveis
      temperatura = newTemp;
      umidade = newHum;



      //controle

      // liga se a umidade ta muito baixa
      if (umidade < UMIDADE_MINIMA && !umidificadorLigado) {
        Serial.println("Umidade baixa! Ligando umidificador");
        digitalWrite(PINO_RELE, HIGH);  //liga
        umidificadorLigado = true;      // avisa que umidificador foi ligado

        // desliga se a umidade ta boa
      } else if (umidade > UMIDADE_MAXIMA && umidificadorLigado) {
        Serial.println("Umidade boa! Desligando umidificador");
        digitalWrite(PINO_RELE, LOW);  //desliga
        umidificadorLigado = false;    //avisa que o umidificador foi desligado
      }
    }
  }
}
