#pragma once
#include <Arduino.h>
#include <LittleFS.h>

class FileHelper {
public:
  static void setReady(bool r);  

  static bool begin(bool formatOnFail = false);

  // Anzahl der Zeilen in einer Datei zählen
  static size_t countLines(const char* path);

  // Bestimmte Zeile (0-basiert) lesen
  static bool readLineAt(const char* path, size_t index, String& out);

  static bool readLineAt(const char* path, size_t index, char* out, size_t outLen);

  // Eine Zeile anhängen (newline wird automatisch ergänzt)
  static bool writeLine(const char* path, const String& line);

  // Datei komplett überschreiben (praktisch für Refresh)
  static bool writeAllLines(const char* path, const std::vector<String>& lines);

  // Datei löschen (für Force-Refresh)
  static bool remove(const char* path);

  // Existenz prüfen
  static bool exists(const char* path);

  // Verzeichnis sicherstellen (rekursiv light)
  static bool ensureDir(const char* path);
private:
  static bool _ready;
};
