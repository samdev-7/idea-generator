#include "Adafruit_Thermal.h"
#include <HardwareSerial.h> 
#include <WiFi.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>

// CHANGE THESE!!
static const char *ssid = "WIFI_SSID";
static const char *pass = "WIFI_PASS";
static const char *openaiKey = "OPENAI_KEY";

const byte rxPin = 16;
const byte txPin = 17;
const int printerBaudrate = 9600;

const byte dtrPin = 4;

const byte buttonPin = 23;
const int debounceDelay = 50; 

const int ledPins[] = {14, 27, 26, 25, 33, 32};
const int numLeds = sizeof(ledPins) / sizeof(ledPins[0]);

HardwareSerial mySerial(1);

Adafruit_Thermal printer(&mySerial, dtrPin);

bool flashLEDs = false;
bool taskInProgress = false;

void setup() {
  Serial.begin(115200);
  Serial.println("Starting...");

  for (int i = 0; i < numLeds; i++) {
    pinMode(ledPins[i], OUTPUT);
    digitalWrite(ledPins[i], LOW);
  }
  pinMode(buttonPin, INPUT_PULLUP);
  offLEDs();

  // Initialize Thermal Printer
  Serial.println("Starting printer...");
  mySerial.begin(printerBaudrate, SERIAL_8N1, rxPin, txPin);  // must be 8N1 mode
  printer.begin();

  Serial.print("Starting WiFi");
  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("");
  
  Serial.println("Printing ready message...");
  printer.justify('C');
  printer.setSize('L');
  printer.println("Ready to go!");
  printer.feed(5);

  printer.setDefault(); // Restore printer to defaults
  printer.sleep();      // Tell printer to sleep
  // printIdea("here's an idea!\n\ntell me a story");
  // printIdea("hi");

  Serial.println("Ready to go!");
  Serial.println();
  Serial.println();
}

void printIdea(String idea) {
  Serial.println("Printing idea...");
  idea = wrapAndPadText(idea, 28);
  printer.wake();
  printer.feed(1);
  printer.println(F("______________________________"));
  
  if (idea.length() <= 28) {
    String pading = "";
    for (int j = 0; j < 28 - idea.length(); j++) {
      pading += " ";
    }
    printer.println("< " + idea + pading + " >");
  } else {
    int i = 0;
    while (true) {
      if (idea.length() <= 28) {
        String pading = "";
        for (int j = 0; j < 28 - idea.length(); j++) {
          pading += " ";
        }
        printer.println("\\ " + idea + pading + " /");
        break;
      } else {
        String line = idea.substring(0, 28);
        if (i == 0) {
          printer.println("/ " + line + " \\");
        } else {
          printer.println("| " + line + " |");
        }
      }
      idea = idea.substring(28);
      i++;
    };
  }

  printer.println(F(" ------------------------------"));
  printer.println(F("         \\"));
  printer.println(F("          \\"));
  printer.println(F("           \\   __"));
  printer.println(F("              / _)"));
  printer.println(F("     _.----._/ /"));
  printer.println(F("    /         /"));
  printer.println(F(" __/ (| | (  |"));
  printer.println(F("/__.-'|_|--|_|"));
  printer.feed(5);
  printer.sleep();

  Serial.println("Done printing");
}

String wrapAndPadText(String input, int columns) {
  String result = "";
  String currentLine = "";
  int start = 0;
  int end = 0;

  while (start < input.length()) {
    while (start < input.length() && input[start] == ' ') {
      start++;
    }
    if (start >= input.length()) break;

    end = input.indexOf(' ', start);
    if (end == -1) end = input.length();

    String word = input.substring(start, end);

    if (currentLine.isEmpty()) {
      currentLine = word;
    } else {
      int possibleLength = currentLine.length() + 1 + word.length();
      if (possibleLength > columns) {
        while (currentLine.length() < columns) {
          currentLine += " ";
        }
        result += currentLine;
        currentLine = word;
      } else {
        currentLine += " " + word;
      }
    }

    start = end + 1;
  }

  // Handle final line
  if (!currentLine.isEmpty()) {
    while (currentLine.length() < columns) {
      currentLine += " ";
    }
    result += currentLine;
  }

  return result;
}

String messageStarters[] = {
  "you could build a",
  "what if you built a",
  "how about a",
  "you could make a",
  "as a dino, i think you should build a",
  "i've been dreaming of creating a",
  "picture this:",
  "oh, oh, oh! a",
  "i dare you to make a"
};
int numStarters = sizeof(messageStarters) / sizeof(messageStarters[0]);

String apiUrl = "https://api.openai.com/v1/chat/completions";

String generateIdea() {
  HTTPClient http;
  Serial.println("Generating idea...");
  String prompt = "You are a software engineer that wants to bring joy through chaos. Come up with something different every time. Please propose a funky simple project that will take under 12 hours to complete in 1 quick sentence. Keep it at less than 15 words and do not use markdown or formatting. Keep your entire response in lowercase to match your vibe. The funkier, stupidier, and sillier your ideas the better. The idea should be useless but still pitch-able. It must have some use, but that use is useless. Think out of the box, and do not propose ideas that do nothing but generate text, like a joke or dance move generator. Random sound effect generators are boring, do not suggest them. Be very creative, do not suggest projects that are too simple. Do not suggest something you've suggested previously. Your response must start with \\\"";
  int randomIndex = random(numStarters);
  prompt += messageStarters[randomIndex] + "\\\"";
  String payload = "{\"model\": \"gpt-4o-mini\",\"messages\": [{\"role\": \"user\",\"content\": [{\"type\": \"text\",\"text\": \"" + prompt + "\"}]}],\"response_format\": {\"type\": \"text\"},\"temperature\": 1,\"max_completion_tokens\": 2048,\"top_p\": 1,\"frequency_penalty\": 0,\"presence_penalty\": 0}";
  Serial.println("Payload: " + payload);
  Serial.println("Sending HTTP request...");

  http.begin(apiUrl);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", "Bearer " + String(openaiKey));

  int httpResponseCode = http.POST(payload);
  Serial.println("Response code: " + String(httpResponseCode));

  if (httpResponseCode == 200) {
    String responsePayload = http.getString();
    Serial.println("Response content: " + responsePayload);

    // Parse JSON response
    DynamicJsonDocument jsonDoc(1024);
    DeserializationError error = deserializeJson(jsonDoc, responsePayload);
    
    if (!error) {
      String outputText = jsonDoc["choices"][0]["message"]["content"];
      Serial.println("Output text: " + outputText);
      return outputText;
    } else {
      Serial.println("JSON parse error");
      return "Error: JSON parsing failed";
    }
  } else {
    Serial.println("Error: " + String(httpResponseCode));
    return "Error: " + String(httpResponseCode);
  }
  
  Serial.println("Done generating idea");
}

void ideaTask(void * parameter) {
  // Perform the HTTPS request and print the idea
  taskInProgress = true;
  flashLEDs = true;
  String idea = generateIdea();
  flashLEDs = false;
  onLEDs();
  printIdea(idea);
  delay(1750);
  offLEDs();

  for (int i = 0; i < 3; i++) {
    delay(250);
    onLEDs();
    delay(250);
    offLEDs();
  }

  taskInProgress = false;
  // End the task when done
  vTaskDelete(NULL);
}

void startIdea() {
  xTaskCreate(
    ideaTask,      // Task function.
    "Idea Task",   // Name of task.
    8192,          // Stack size in words.
    NULL,          // Task input parameter.
    1,             // Priority of the task.
    NULL           // Task handle.
  );
}

void toggleLEDs() {
  int ledIndex = random(numLeds);
  int ledPin = ledPins[ledIndex];

  for (int i = 0; i < numLeds; i++) {
    if (i == ledIndex) {
      digitalWrite(ledPins[i], HIGH);
    } else {
      digitalWrite(ledPins[i], LOW);
    }
  }
}

void offLEDs() {
  for (int i = 0; i < numLeds; i++) {
    digitalWrite(ledPins[i], LOW);
  }
}

void onLEDs() {
  for (int i = 0; i < numLeds; i++) {
    digitalWrite(ledPins[i], HIGH);
  }
}

int lastState = HIGH;

void loop() {
  if (flashLEDs) {
    toggleLEDs();
    delay(100);
  } else if (!taskInProgress){
    int buttonState = digitalRead(buttonPin);
    if (buttonState == LOW && lastState == LOW) {
      Serial.println("BUTTON PRESSED");
      startIdea();
    }
    lastState = buttonState;
    delay(debounceDelay);
  }
}
