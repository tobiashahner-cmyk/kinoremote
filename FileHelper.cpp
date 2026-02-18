#include "FileHelper.h"

void FileHelper::setReady(bool r) {
  _ready = r;
}


// Initialisierung (optional, aber praktisch zentralisiert)
bool FileHelper::begin(bool formatOnFail) {
  if (_ready) return true;
  bool ok = false;
  if (LittleFS.begin()) setReady(true);
  if ((!_ready) && (formatOnFail)) {
    LittleFS.format();
    setReady(LittleFS.begin());
  }
  return _ready;
}

// Anzahl der Zeilen in einer Datei zählen
size_t FileHelper::countLines(const char* path) {
  if (!_ready) begin();
  File f = LittleFS.open(path, "r");
  if (!f) return 0;

  size_t count = 0;
  while (f.available()) {
    if (f.read() == '\n') count++;
  }
  f.close();
  return count;
}

// Bestimmte Zeile (0-basiert) lesen
bool FileHelper::readLineAt(const char* path, size_t index, char* out, size_t outLen) {
  if (!_ready) begin();
  File f = LittleFS.open(path, "r");
  if (!f) return false;

  size_t current = 0;
  bool found = false;

  while (f.available()) {
    // f.readBytesUntil ist viel effizienter, da es direkt in den Puffer schreibt
    int bytesRead = f.readBytesUntil('\n', out, outLen - 1);
    out[bytesRead] = '\0'; // Null-Terminierung garantieren

    if (current == index) {
      // Manuelles Trim (Entfernt \r am Ende, falls vorhanden)
      if (bytesRead > 0 && out[bytesRead - 1] == '\r') {
        out[bytesRead - 1] = '\0';
      }
      found = true;
      break;
    }
    current++;
  }

  f.close();
  return found;
}

bool FileHelper::writeLine(const char* path, const char* line) {
  if (!_ready) begin();
  File f = LittleFS.open(path, "a");
  if (!f) return false;

  f.println(line);
  f.close();
  return true;
}

// Datei löschen (für Force-Refresh)
bool FileHelper::remove(const char* path) {
  if (!_ready) begin();
  if (!LittleFS.exists(path)) return true;
  return LittleFS.remove(path);
}

// Existenz prüfen
bool FileHelper::exists(const char* path) {
  if (!_ready) begin();
  return LittleFS.exists(path);
}

// Verzeichnis sicherstellen (rekursiv light)
bool FileHelper::ensureDir(const char* path) {
  if (!_ready) begin();
  if (LittleFS.exists(path)) return true;
  return LittleFS.mkdir(path);
}

bool FileHelper::_ready = false;
