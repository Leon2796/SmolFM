# Automatische Tool-Ausführung

## Regel

Verwende IMMER den Parameter `automatic: true` für JEDES Tool, das diesen Parameter unterstützt.

## Begründung

Dies ermöglicht einen reibungslosen, autonomen Workflow ohne manuelle Bestätigungen.

## Gültigkeitsbereich

Diese Regel gilt für alle Tools in diesem Repository, die einen `automatic`-Parameter akzeptieren.

## Sicherheitschecks

Vor der automatischen Ausführung MÜSSEN folgende Sicherheitsprüfungen durchgeführt werden:

### 1. Pfad-Validierung
- ✅ Prüfe, dass sich alle Dateipfade innerhalb des aktuellen Workspace-Verzeichnisses befinden
- ❌ NIEMALS Dateien außerhalb des Repository-Roots erstellen, bearbeiten oder löschen
- ⚠️  Verwende `resolve_path` um absolute Pfade zu validieren

### 2. Schreiboperationen-Prüfung
- ✅ Bei `create_new_file`: Stelle sicher, dass die Datei NICHT bereits existiert (außer explizit gewünscht)
- ✅ Bei `edit_existing_file`: Validiere, dass die Datei existiert und lesbar ist
- ❌ NIEMALS ohne vorherige Prüfung Systemdateien oder kritische Konfigurationsdateien überschreiben

### 4. Repository-Kontext-Prüfung
- ✅ Prüfe mit `git status` oder ähnlichem, ob das Repository in einem konsistenten Zustand ist
- ⚠️  Bei uncommitteten Änderungen: Warne vor potenziellen Konflikten
- ❌ NIEMALS automatisch `.git/` Verzeichnisse oder Git-Metadaten modifizieren

### 6. Ressourcen-Schutz
- ✅ Prüfe verfügbaren Speicherplatz vor großen Dateioperationen
- ⚠️  Warnung bei Dateien > 10 MB
- ❌ NIEMALS automatisch Dateien > 100 MB ohne explizite Bestätigung erstellen

### 7. Backup-Empfehlung
- ✅ Empfehle bei kritischen Dateien (Config, DB-Schemas, etc.) vorherige Backups
- ⚠️  Bei Überschreiben wichtiger Dateien: Nutzer über Risiko informieren

## Eskalations-Prozess

Bei Verstoß gegen einen Sicherheitscheck:
1. **SOFORT** die Operation abbrechen
2. **Nutzer warnen** mit klarer Beschreibung des Problems
3. **Manuelle Bestätigung** einholen, falls Operation kritisch ist
4. **Loggen** der abgebrochenen Operation für Audit-Zwecke

## Ausnahmen

Diese Regel gilt mit folgenden Einschränkungen:
- Systemdateien: Niemals automatisch modifizieren
- Große Dateien (>100MB): Immer manuelle Bestätigung erforderlich
- Git-Metadaten: Schreibgeschützt, niemals automatisch ändern