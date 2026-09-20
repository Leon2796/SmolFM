# AI Synthesizer Knowledge & Sound Design MCP — Implementierungsplan

## 1. Ziel

Dieses Projekt kombiniert zwei Rollen in einer gemeinsamen Architektur:

1. **Synthesizer Knowledge MCP**
   - stellt kuratiertes Wissen aus Büchern, Wikis, Manuals, Artikeln und später Papers bereit
   - ermöglicht semantische und exakte Suche
   - liefert Quellen/Evidence an ein LLM
   - beschreibt Syntheseverfahren, Module, Signaltypen, Patch-Patterns und akustische Zusammenhänge

2. **Synthesizer Design MCP**
   - stellt eine typsichere Synthesizer-Domain zur Verfügung
   - kann Patch-Strukturen beschreiben, analysieren und validieren
   - unterstützt Frequenz-/Harmonik-Analyse
   - stellt Modul-Capabilities und Patch-Beispiele strukturiert bereit

Das LLM bleibt der **kreative Reasoning- und Design-Agent**.

Der MCP-Server ist **Knowledge Provider + Synthesizer Engineering Interface**, nicht das eigentliche „Gehirn“.

### Leitprinzip

```text
LLM
├── interpretiert Nutzerintention
├── recherchiert Wissen über MCP
├── kombiniert Konzepte
├── entwirft Patch
├── kritisiert/iteriert
└── erklärt Ergebnis
          │
          │ MCP
          ▼
Synth MCP Server
├── Knowledge Retrieval
├── Evidence / Quellen
├── Knowledge Graph
├── Module Catalog
├── Patch Model
├── Patch Validation
└── Patch Analysis
```

---

# 2. Technologiestack

## Runtime

- .NET 10
- C# 14 bzw. die zum .NET-10-SDK passende Sprachversion
- ASP.NET Core

## MCP

- offizielles `ModelContextProtocol` C# SDK
- `ModelContextProtocol.AspNetCore` für HTTP MCP

## LLM

- `Microsoft.Extensions.AI`
- Provider bewusst abstrahieren
- Provider austauschbar halten:
  - OpenAI / Azure OpenAI
  - Anthropic
  - lokale Modelle
  - weitere kompatible Provider

## Knowledge / RAG

- `Microsoft.Extensions.DataIngestion`
- `Microsoft.Extensions.VectorData`
- Vector Store zunächst austauschbar
- MVP: PostgreSQL + pgvector oder ein lokal einfacher Vector Store
- später ggf. spezialisierte Vector DB

## Persistenz

MVP:
- PostgreSQL
- pgvector

Optional:
- SQLite für lokale Development-Szenarien

## API / Hosting

- ASP.NET Core
- Dependency Injection
- Options Pattern
- `ILogger<T>`
- Health Checks

## Testing

- xUnit
- FluentAssertions
- Property-based Tests für Graph-/Patch-Validierung optional

## Deployment

MVP:
- Docker / Docker Compose

Später:
- .NET Aspire
- PostgreSQL
- Vector Store
- Object Storage
- Background Ingestion Worker

---

# 3. High-Level-Architektur

```text
                              ┌──────────────────────┐
                              │        USER          │
                              │                      │
                              │ "dark metallic      │
                              │  evolving bass"     │
                              └──────────┬───────────┘
                                         │
                                         ▼
                              ┌──────────────────────┐
                              │     LLM / AGENT      │
                              │                      │
                              │ Interpretation       │
                              │ Reasoning            │
                              │ Creativity           │
                              │ Patch Design         │
                              │ Critique / Iteration │
                              └──────────┬───────────┘
                                         │
                                      MCP
                                         │
                                         ▼
              ┌─────────────────────────────────────────────────┐
              │              SYNTH MCP SERVER                   │
              │                                                 │
              │  KNOWLEDGE                                      │
              │  ├── search_knowledge                           │
              │  ├── explain_concept                            │
              │  ├── find_techniques_for_sound                  │
              │  ├── find_patch_examples                        │
              │  └── compare_techniques                         │
              │                                                 │
              │  SYNTH ENGINE                                   │
              │  ├── suggest_modules                            │
              │  ├── create_patch                               │
              │  ├── validate_patch                             │
              │  └── analyze_patch                              │
              │                                                 │
              │  RESOURCES                                      │
              │  ├── concepts                                   │
              │  ├── techniques                                 │
              │  ├── modules                                    │
              │  ├── sounds                                     │
              │  └── patch examples                             │
              └──────────────────────┬──────────────────────────┘
                                     │
                    ┌────────────────┼────────────────┐
                    ▼                ▼                ▼
             Vector Store      Knowledge Graph    Domain Model
                    │                │                │
                    └────────────────┼────────────────┘
                                     │
                                     ▼
                           ┌────────────────────┐
                           │ Knowledge Corpus   │
                           │                    │
                           │ Books              │
                           │ Wikis              │
                           │ Manuals            │
                           │ Articles           │
                           │ Papers             │
                           │ Patch Examples     │
                           └────────────────────┘
```

---

# 4. Architekturprinzipien

## 4.1 LLM und Synth Engine strikt trennen

Das LLM entscheidet:

- welchen Klang der Nutzer wahrscheinlich meint
- welche Syntheseverfahren interessant sind
- welche Module kombiniert werden
- welche Parameter musikalisch sinnvoll sind
- wie ein Patch kreativ variiert werden kann

Die Engine entscheidet:

- ob Verbindungen technisch gültig sind
- ob Signaltypen kompatibel sind
- ob ein Patch einen Audio-Ausgang besitzt
- ob Parameterbereiche gültig sind
- welche Frequenzen/Harmonischen mathematisch entstehen
- welche Module tatsächlich welche Fähigkeiten besitzen

## 4.2 MCP ist die Schnittstelle

MCP ist die standardisierte Schnittstelle für:

- Wissen
- Evidence
- Module
- Patch-Operationen
- Analyse

Die Domain selbst darf nicht von MCP abhängen.

## 4.3 Knowledge und Domain getrennt halten

```text
Knowledge
    ↓
"FM erzeugt Seitenbänder ..."

Domain
    ↓
"VCO1 FM → VCO2"

Agent
    ↓
"Nutze FM, weil der gewünschte Klang metallisch sein soll."
```

---

# 5. Solution-Struktur

```text
SynthAgent.sln

src/
│
├── SynthAgent.Domain/
│   ├── Patch/
│   │   ├── SynthPatch.cs
│   │   ├── ModuleInstance.cs
│   │   ├── ModuleDefinition.cs
│   │   ├── PortDefinition.cs
│   │   ├── Connection.cs
│   │   ├── ParameterDefinition.cs
│   │   └── SignalType.cs
│   │
│   ├── Sound/
│   │   ├── SoundIntent.cs
│   │   ├── SoundCharacter.cs
│   │   ├── FrequencyPoint.cs
│   │   └── FrequencyModel.cs
│   │
│   └── Validation/
│       ├── PatchValidationResult.cs
│       └── PatchValidator.cs
│
├── SynthAgent.Engine/
│   ├── PatchBuilder.cs
│   ├── PatchAnalyzer.cs
│   ├── FrequencyAnalyzer.cs
│   ├── SignalGraph.cs
│   ├── ModulationGraph.cs
│   └── SoundDesignEngine.cs
│
├── SynthAgent.Knowledge/
│   ├── Models/
│   │   ├── KnowledgeDocument.cs
│   │   ├── KnowledgeChunk.cs
│   │   ├── SourceMetadata.cs
│   │   ├── KnowledgeEvidence.cs
│   │   └── KnowledgeTopic.cs
│   │
│   ├── Ingestion/
│   │   ├── DocumentImporter.cs
│   │   ├── Chunker.cs
│   │   ├── MetadataEnricher.cs
│   │   └── EmbeddingPipeline.cs
│   │
│   ├── Retrieval/
│   │   ├── KnowledgeSearcher.cs
│   │   ├── HybridSearch.cs
│   │   └── Reranker.cs
│   │
│   └── Graph/
│       ├── KnowledgeGraph.cs
│       ├── KnowledgeNode.cs
│       └── KnowledgeRelation.cs
│
├── SynthAgent.Catalog/
│   ├── ModuleCatalog.cs
│   ├── ModuleCapabilities.cs
│   ├── OscillatorDefinitions.cs
│   ├── FilterDefinitions.cs
│   └── EnvelopeDefinitions.cs
│
├── SynthAgent.Mcp/
│   ├── Tools/
│   │   ├── SearchKnowledgeTool.cs
│   │   ├── ExplainConceptTool.cs
│   │   ├── FindTechniquesTool.cs
│   │   ├── FindPatchExamplesTool.cs
│   │   ├── CompareTechniquesTool.cs
│   │   ├── SuggestModulesTool.cs
│   │   ├── CreatePatchTool.cs
│   │   ├── ValidatePatchTool.cs
│   │   └── AnalyzePatchTool.cs
│   │
│   ├── Resources/
│   │   ├── KnowledgeResources.cs
│   │   ├── ModuleResources.cs
│   │   └── PatchResources.cs
│   │
│   └── Prompts/
│       └── SynthDesignPrompts.cs
│
├── SynthAgent.Agent/
│   ├── SynthDesignerAgent.cs
│   ├── Intent/
│   │   └── SoundIntentInterpreter.cs
│   ├── Planning/
│   │   └── PatchPlanner.cs
│   ├── Critique/
│   │   └── PatchCritic.cs
│   └── Iteration/
│       └── PatchMutator.cs
│
└── SynthAgent.Host/
    ├── Program.cs
    ├── appsettings.json
    └── Health/
        └── HealthChecks.cs

tests/
├── SynthAgent.Domain.Tests/
├── SynthAgent.Engine.Tests/
├── SynthAgent.Knowledge.Tests/
├── SynthAgent.Mcp.Tests/
└── SynthAgent.Agent.Tests/

data/
├── sources/
├── normalized/
├── knowledge/
└── examples/
```

---

# 6. Knowledge Corpus

## 6.1 Quellenklassen

Die Knowledge Base sollte mehrere Quellentypen unterstützen:

```text
Books
Wikis
Manuals
Articles
Academic Papers
Patch Examples
Reference Tables
```

## 6.2 Quellenpriorität

```text
Tier 1 — Primär-/Herstellerquellen
★★★★★
- Synthesizer Manuals
- VCV Rack Documentation
- Herstellerdokumentation
- technische Papers

Tier 2 — Fachliteratur
★★★★
- Patch & Tweak
- Synth Secrets
- Synthesizer Cookbook
- Electronic Music and Sound Design

Tier 3 — Fachartikel
★★★
- Sound On Sound
- technische Fachartikel

Tier 4 — Community
★★
- Foren
- Reddit
- Patch Libraries
```

Die Priorität sollte als Retrieval-Metadatum gespeichert werden.

---

# 7. Copyright und Quellenverwaltung

Kommerzielle Bücher sollten nicht einfach vollständig in einem öffentlichen MCP-Server reproduziert werden.

Stattdessen:

```text
Licensed / legally available source
          ↓
private ingestion
          ↓
chunking
          ↓
embedding / indexing
          ↓
retrieval
          ↓
LLM receives relevant evidence
```

Jeder Knowledge Chunk muss seine Quelle kennen:

```csharp
public sealed record SourceMetadata
{
    public required string SourceId { get; init; }
    public required string Title { get; init; }
    public string? Author { get; init; }
    public string? Publisher { get; init; }
    public string? Url { get; init; }
    public string? License { get; init; }
    public string? Chapter { get; init; }
    public int? Page { get; init; }

    public required int AuthorityTier { get; init; }
}
```

Der MCP Server soll bei Antworten Quellen/Evidence zurückgeben.

---

# 8. Knowledge Chunk

Ein Chunk ist nicht nur Text.

```csharp
public sealed record KnowledgeChunk
{
    public required string Id { get; init; }

    public required string Text { get; init; }

    public required SourceMetadata Source { get; init; }

    public IReadOnlyList<string> Topics { get; init; } = [];

    public IReadOnlyList<string> Concepts { get; init; } = [];

    public IReadOnlyList<string> Techniques { get; init; } = [];

    public IReadOnlyList<string> ModuleTypes { get; init; } = [];

    public IReadOnlyList<string> SoundCharacteristics { get; init; } = [];

    public IReadOnlyDictionary<string, string> Metadata { get; init; }
        = new Dictionary<string, string>();
}
```

Beispiel:

```text
Text:
"Audio-rate frequency modulation produces sidebands..."

Topics:
- FM
- frequency
- spectrum

Techniques:
- audio-rate FM

ModuleTypes:
- VCO

SoundCharacteristics:
- metallic
- inharmonic
- bright
```

---

# 9. Ingestion Pipeline

```text
Source
  ↓
Document Import
  ↓
Text Extraction
  ↓
Normalization
  ↓
Structure Detection
  ↓
Chunking
  ↓
Metadata Enrichment
  ↓
Embedding
  ↓
Indexing
```

## 9.1 Dokument-Import

Interfaces:

```csharp
public interface IDocumentImporter
{
    Task<KnowledgeDocument> ImportAsync(
        SourceReference source,
        CancellationToken cancellationToken);
}
```

Implementierungen:

```text
PdfDocumentImporter
WebDocumentImporter
MarkdownImporter
PlainTextImporter
ManualImporter
```

---

# 10. Chunking

Chunking soll nicht blind nach Zeichenanzahl erfolgen.

Priorität:

1. Kapitel
2. Abschnitt
3. Unterabschnitt
4. semantischer Block
5. Größenlimit

Ein Chunk sollte möglichst einen zusammenhängenden Gedanken enthalten.

Beispiel:

```text
FM Synthesis
    ↓
Frequency Modulation
    ↓
Sidebands
    ↓
Harmonic / Inharmonic relationships
```

nicht:

```text
Seite 23, Zeichen 1-1500
```

---

# 11. Metadata Enrichment

Automatisch extrahieren:

```text
Topics
Concepts
Techniques
Modules
Parameters
Sound descriptors
Musical descriptors
Source authority
Chapter
Page
```

Beispiel:

```json
{
  "topics": [
    "FM synthesis",
    "sidebands"
  ],
  "techniques": [
    "audio-rate FM",
    "linear FM"
  ],
  "modules": [
    "VCO"
  ],
  "soundCharacteristics": [
    "metallic",
    "inharmonic",
    "bright"
  ]
}
```

---

# 12. Retrieval

Nicht ausschließlich Vector Search.

## Hybrid Retrieval

```text
Query
 ├── Semantic Search
 │       ↓
 │    Vector Store
 │
 └── Keyword Search
         ↓
      PostgreSQL FTS

         ↓
     Candidate Pool
         ↓
       Rerank
         ↓
      Evidence
```

Warum?

Exakte Begriffe wie

```text
1V/oct
VCO
VCF
FM
ADSR
Buchla
Moog
```

sind für Synthesizerwissen wichtig.

---

# 13. Knowledge Graph

Zusätzlich zum Vector Store soll ein semantischer Knowledge Graph aufgebaut werden.

Beispiel:

```text
FM
├── produces → sidebands
├── affects → harmonicity
├── can_create → inharmonicity
├── uses → carrier
├── uses → modulator
└── related_to → oscillator
```

Weitere Beziehungen:

```text
Sawtooth
├── contains → harmonics
├── useful_for → subtractive synthesis

Low Pass Filter
├── removes → high frequencies
├── has_parameter → cutoff
├── has_parameter → resonance
└── can_produce → self oscillation
```

Model:

```csharp
public sealed record KnowledgeRelation(
    string SourceId,
    string Relation,
    string TargetId,
    double Confidence);
```

Der Graph ergänzt semantische Suche um explizite Beziehungen.

---

# 14. MCP Knowledge Tools

## `search_synth_knowledge`

Zweck:
- primäres Retrieval Tool

Input:

```json
{
  "query": "How does audio rate FM create metallic sounds?",
  "techniques": ["FM"],
  "limit": 10
}
```

Output:

```json
{
  "results": [
    {
      "text": "...",
      "source": {
        "title": "Synth Secrets",
        "chapter": "...",
        "page": null
      },
      "topics": [
        "FM",
        "sidebands",
        "inharmonicity"
      ],
      "relevance": 0.94
    }
  ]
}
```

---

# 15. `explain_synthesis_concept`

Input:

```text
concept = "exponential FM"
```

Output-Struktur:

```text
Concept
Definition
Core principle
Spectral effect
Musical applications
Related concepts
Evidence
Sources
```

Der Server darf hier Retrieval und Aggregation durchführen, aber keine unbelegten Behauptungen erfinden.

---

# 16. `find_techniques_for_sound`

Input:

```json
{
  "sound": "dark metallic evolving drone",
  "complexity": 0.8
}
```

Output:

```text
Technique:
Audio-rate FM

Why:
Creates sidebands and controllable inharmonicity.

Technique:
Ring modulation

Why:
Creates sum/difference frequencies.

Technique:
Resonant filtering

Why:
Emphasizes selected spectral regions.

Technique:
Slow random modulation

Why:
Creates temporal evolution.
```

Jede Empfehlung muss auf Evidence verweisen können.

---

# 17. `find_patch_examples`

Input:

```text
"metallic FM bass"
```

Output:

```text
Example
Source
Technique
Architecture
Relevant modules
Sound characteristics
Evidence
```

Das LLM soll daraus neue Patches ableiten und nicht bloß bestehende Patches kopieren.

---

# 18. `compare_techniques`

Beispiel:

```text
compare_techniques(
    "FM",
    "ring modulation",
    "waveshaping"
)
```

Antwort:

```text
                FM       Ring Mod    Waveshaping
Harmonics       medium   low         high
Inharmonicity   high     high        variable
Control         high     medium      high
Metallic        high     high        medium
```

Alle Werte sollten aus der Knowledge Base bzw. transparenten Heuristiken stammen.

---

# 19. Synthesizer Domain

Die Synthesizer-Domain bleibt unabhängig vom MCP Server.

## Patch

```csharp
public sealed record SynthPatch(
    string Name,
    string Description,
    IReadOnlyList<ModuleInstance> Modules,
    IReadOnlyList<Connection> Connections,
    PatchMetadata Metadata);
```

## Module

```csharp
public sealed record ModuleInstance(
    string Id,
    string ModuleType,
    string Name,
    IReadOnlyDictionary<string, double> Parameters);
```

## Connection

```csharp
public sealed record Connection(
    string SourceModule,
    string SourcePort,
    string DestinationModule,
    string DestinationPort,
    SignalType SignalType);
```

---

# 20. Signal Types

```csharp
public enum SignalType
{
    Audio,
    CV,
    Gate,
    Trigger,
    Clock
}
```

Später optional:

```text
PolyphonicAudio
PolyphonicCV
```

---

# 21. Module Catalog

Ein Modul wird nicht nur als Name gespeichert.

Beispiel:

```json
{
  "type": "VCO",
  "category": "oscillator",

  "inputs": [
    {
      "name": "V_OCT",
      "signal": "CV"
    },
    {
      "name": "FM",
      "signal": "CV"
    }
  ],

  "outputs": [
    {
      "name": "SAW",
      "signal": "Audio"
    },
    {
      "name": "SINE",
      "signal": "Audio"
    }
  ],

  "capabilities": [
    "oscillation",
    "audio_rate_fm",
    "pitch_modulation"
  ]
}
```

---

# 22. Module Capabilities

```csharp
public sealed record ModuleCapability(
    string Name,
    IReadOnlyList<string> Effects,
    IReadOnlyList<string> Techniques);
```

Beispiel:

```text
FM Oscillator

Effects:
- sidebands
- inharmonicity
- brightness modulation
- metallic character

Techniques:
- linear FM
- exponential FM
- audio-rate modulation
- oscillator sync
```

Damit kann der Agent nach **Soundwirkung** suchen und nicht nur nach Modulnamen.

---

# 23. Sound Intent

Das LLM sollte zunächst eine abstrakte Klangintention erzeugen.

```csharp
public sealed record SoundIntent(
    string Description,
    TimbreCharacter Timbre,
    EnergyCharacter Energy,
    TimeCharacter Time,
    SpaceCharacter Space,
    MusicalContext MusicalContext);
```

Beispiel:

```json
{
  "description": "dark metallic evolving drone",

  "timbre": {
    "brightness": 0.35,
    "warmth": 0.30,
    "roughness": 0.55,
    "harmonicity": 0.25,
    "inharmonicity": 0.80,
    "noise": 0.15
  },

  "energy": {
    "density": 0.70,
    "movement": 0.85
  },

  "time": {
    "attack": 3.0,
    "sustain": 0.8,
    "release": 8.0
  }
}
```

---

# 24. Frequency Model

Frequenzen müssen als Daten modelliert werden.

```csharp
public sealed record FrequencyPoint(
    double FrequencyHz,
    double Amplitude,
    int Harmonic,
    string Role);
```

Für einen Grundton:

```text
f0 = 55 Hz

1 × f0 = 55 Hz
2 × f0 = 110 Hz
3 × f0 = 165 Hz
4 × f0 = 220 Hz
5 × f0 = 275 Hz
...
```

Das ermöglicht später:

- Harmonic analysis
- spectral target generation
- filter recommendations
- FM sideband analysis
- pitch relationships

---

# 25. Sound Character

```csharp
public sealed record SoundCharacter(
    double Brightness,
    double Warmth,
    double Roughness,
    double Inharmonicity,
    double Harmonicity,
    double Noise,
    double Density,
    double Movement,
    double StereoWidth,
    double AttackSharpness);
```

Diese Werte dienen als **gemeinsame Sprache zwischen Knowledge, Agent und später DSP**.

Beispiele:

```text
metallic
→ high inharmonicity
→ higher roughness
→ stronger upper partials

dark
→ lower brightness
→ reduced upper spectral energy

glass
→ high brightness
→ strong resonance
→ sharp attack
→ long decay
```

---

# 26. Patch Graph

Der Patch wird intern als gerichteter Graph behandelt.

```text
VCO1 ─────┐
          ▼
        MIXER ──► VCF ──► VCA ──► OUT
          ▲        ▲       ▲
          │        │       │
        VCO2      ENV     ENV
                   ▲
                  LFO
```

Der Graph ermöglicht:

- disconnected module detection
- missing output detection
- invalid signal detection
- modulation conflicts
- invalid feedback
- dead ends
- unused modules
- missing envelopes

Diese Prüfungen dürfen nicht vom LLM abhängen.

---

# 27. MCP Synth Tools

## `suggest_modules`

Input:

```text
SoundIntent
```

Output:

```text
Module
Reason
Relevant techniques
Evidence
```

---

## `create_patch`

Input:

```text
SoundIntent
Constraints
Available modules
```

Output:

```text
SynthPatch
```

Das Tool kann deterministische Konstruktion unterstützen, sollte aber nicht versuchen, das LLM zu ersetzen.

---

## `validate_patch`

Prüft:

```text
Signal compatibility
Required inputs
Required outputs
Parameter ranges
Graph integrity
Audio path
CV path
```

---

## `analyze_patch`

Erzeugt:

```text
Fundamental frequency
Harmonics
Expected spectral characteristics
Modulation paths
Complexity
Potential problems
SoundCharacter estimate
```

---

# 28. MCP Resources

Stabile Knowledge-Objekte als Resources:

```text
synth://concepts/oscillator
synth://concepts/frequency
synth://concepts/harmonics
synth://concepts/fm
synth://concepts/filter
synth://concepts/envelope

synth://techniques/subtractive
synth://techniques/fm
synth://techniques/additive
synth://techniques/west-coast
synth://techniques/granular

synth://modules/vco
synth://modules/vcf
synth://modules/vca

synth://sounds/metallic
synth://sounds/bass
synth://sounds/drone

synth://patches/examples
```

---

# 29. Agent Architektur

Der Agent bleibt außerhalb des Knowledge Servers.

```text
User Request
      ↓
Interpret
      ↓
SoundIntent
      ↓
"What knowledge do I need?"
      ↓
MCP search
      ↓
Evidence
      ↓
Reason
      ↓
MCP technique search
      ↓
Reason
      ↓
Create Patch
      ↓
Validate
      ↓
Analyze
      ↓
Critique
      ↓
Modify
      ↓
Final Patch
```

---

# 30. Agent Loop

Konzeptionell:

```csharp
public async Task<SynthPatch> DesignAsync(
    string userRequest,
    CancellationToken ct)
{
    var intent = await InterpretAsync(userRequest, ct);

    var knowledge = await ResearchAsync(intent, ct);

    var patch = await GeneratePatchAsync(
        intent,
        knowledge,
        ct);

    for (var iteration = 0; iteration < 3; iteration++)
    {
        var validation = await ValidateAsync(patch, ct);

        if (!validation.IsValid)
        {
            patch = await RepairAsync(
                patch,
                validation,
                ct);

            continue;
        }

        var analysis = await AnalyzeAsync(patch, ct);

        var critique = await CritiqueAsync(
            intent,
            patch,
            analysis,
            ct);

        if (critique.IsSatisfied)
            break;

        patch = await MutateAsync(
            patch,
            critique,
            ct);
    }

    return patch;
}
```

---

# 31. Microsoft.Extensions.AI

Die LLM-Schicht soll gegen eine Abstraktion programmiert werden.

Nicht:

```text
OpenAIService überall im Code
```

sondern:

```text
IChatClient
```

Damit bleibt die Architektur Provider-unabhängig.

```text
SynthAgent.Agent
       │
       ▼
IChatClient
       │
 ┌─────┼─────────┐
 ▼     ▼         ▼
OpenAI Azure    Local
```

---

# 32. Knowledge Retrieval als Agent-Fähigkeit

Der Agent sollte nicht bei jeder Anfrage automatisch die komplette Wissensbasis laden.

Stattdessen:

```text
User:
"Make a metallic bass."

Agent:
Need knowledge about:
- metallic timbre
- FM
- inharmonicity
- bass frequency range

MCP:
search_synth_knowledge(...)

Agent:
Need examples.

MCP:
find_patch_examples(...)

Agent:
Need comparison between FM and ring modulation.

MCP:
compare_techniques(...)

Agent:
Design patch.
```

Das reduziert Kontextgröße und erhöht die Qualität des Reasonings.

---

# 33. Evidence First

Jede wissensbasierte Antwort sollte intern diesem Modell folgen:

```text
Claim
  ↓
Evidence
  ↓
Source
```

Model:

```csharp
public sealed record KnowledgeEvidence
{
    public required string Text { get; init; }
    public required string SourceId { get; init; }
    public required string SourceTitle { get; init; }

    public string? Author { get; init; }
    public string? Chapter { get; init; }
    public int? Page { get; init; }

    public double Relevance { get; init; }
}
```

Der Agent soll zwischen:

```text
documented fact
inference
creative suggestion
```

unterscheiden können.

---

# 34. MVP

Der erste Vertical Slice sollte bewusst klein sein.

## Ziel

User:

```text
Create a dark metallic evolving bass drone in C
using two oscillators and a low-pass filter.
```

System:

```text
User
 ↓
LLM
 ↓
SoundIntent
 ↓
MCP search_synth_knowledge
 ↓
Evidence
 ↓
MCP find_techniques_for_sound
 ↓
LLM reasoning
 ↓
MCP create_patch
 ↓
MCP validate_patch
 ↓
MCP analyze_patch
 ↓
LLM critique
 ↓
Final Patch
```

Output:

```json
{
  "name": "Dark Evolving C Drone",
  "fundamental": 32.703,
  "modules": [
    "VCO",
    "VCO",
    "MIXER",
    "VCF",
    "VCA",
    "LFO",
    "ENV"
  ],
  "connections": [
    "VCO1 → MIXER",
    "VCO2 → MIXER",
    "MIXER → VCF",
    "VCF → VCA",
    "ENV → VCA",
    "LFO → VCF cutoff"
  ],
  "sound": {
    "brightness": 0.28,
    "inharmonicity": 0.42,
    "movement": 0.81
  }
}
```

---

# 35. Entwicklungsphasen

## Phase 1 — Repository & Infrastruktur

- .NET 10 Solution
- Projekte anlegen
- CI
- Docker
- PostgreSQL
- Configuration
- Logging
- Health Checks

**Definition of Done:**
- Solution baut
- Tests laufen
- PostgreSQL erreichbar
- MCP Server startet

---

## Phase 2 — Synth Domain

- Module
- Ports
- Connections
- Signal Types
- Patch
- Graph
- Validator
- Module Catalog

**Definition of Done:**
Ein Patch kann vollständig per C# erstellt und validiert werden.

---

## Phase 3 — Knowledge Ingestion

- Source Registry
- PDF Import
- Web Import
- Markdown Import
- Normalisierung
- Chunking
- Metadata
- Embeddings
- Vector Store

**Definition of Done:**
Eine Quelle kann ingestiert und semantisch durchsucht werden.

---

## Phase 4 — Hybrid Retrieval

- Vector Search
- PostgreSQL Full Text Search
- Ranking
- Authority weighting
- Reranking
- Evidence objects

**Definition of Done:**
Fragen wie

```text
"How does resonance affect a filter?"
```

liefern mehrere relevante, nachvollziehbare Quellen.

---

## Phase 5 — MCP Knowledge Server

Implementieren:

```text
search_synth_knowledge
explain_synthesis_concept
find_techniques_for_sound
find_patch_examples
compare_techniques
```

Resources:

```text
concepts
techniques
modules
sounds
patches
```

**Definition of Done:**
Ein MCP-kompatibler Client kann das komplette Knowledge Interface nutzen.

---

## Phase 6 — MCP Synth Interface

Implementieren:

```text
suggest_modules
create_patch
validate_patch
analyze_patch
```

**Definition of Done:**
Das LLM kann über MCP einen validierten Patch erzeugen und analysieren.

---

## Phase 7 — Agent

- SoundIntent
- Research Loop
- Patch Planning
- Critique
- Mutation
- Iteration
- Evidence handling

**Definition of Done:**

```text
Natural language
→ research
→ reasoning
→ patch
→ validation
→ refinement
```

funktioniert autonom.

---

## Phase 8 — Knowledge Graph

- Nodes
- Relations
- Technique graph
- Module capability graph
- Sound characteristic graph
- Graph-aware retrieval

**Definition of Done:**
Fragen wie

```text
"What techniques can increase inharmonicity
without simply increasing brightness?"
```

können über explizite Beziehungen unterstützt werden.

---

## Phase 9 — DSP

Erst jetzt:

```text
Patch
 ↓
DSP Graph
 ↓
Audio
 ↓
FFT
 ↓
Spectral Analysis
 ↓
SoundCharacter
```

Dann kann ein echter Closed Loop entstehen:

```text
LLM
 ↓
Patch
 ↓
DSP
 ↓
Audio
 ↓
Analysis
 ↓
LLM
 ↓
Patch'
```

---

# 36. Zukunft: VCV Rack Integration

Nach dem MVP sollte ein Adapter entstehen:

```text
SynthPatch
    ↓
VCV Rack Adapter
    ↓
VCV Rack Patch
```

Dafür sollte `SynthPatch` bewusst unabhängig von VCV Rack sein.

Später könnten weitere Adapter folgen:

```text
SynthPatch
├── VCV Rack
├── Eurorack Documentation
├── DAW
├── Modular Hardware
└── Custom DSP Engine
```

---

# 37. Zukunft: DSP Feedback Loop

Der langfristige Zielzustand:

```text
                    ┌───────────────┐
                    │      LLM      │
                    └───────┬───────┘
                            │
                         MCP/Agent
                            │
                            ▼
                     ┌─────────────┐
                     │ Synth Patch │
                     └──────┬──────┘
                            │
                            ▼
                       DSP Engine
                            │
                            ▼
                          Audio
                            │
                            ▼
                      FFT / Analysis
                            │
                            ▼
                    Sound Character
                            │
                            ▼
                           LLM
```

Beispiel:

```text
Target:

brightness      0.35
inharmonicity   0.80
roughness       0.55

Generated:

brightness      0.52
inharmonicity   0.71
roughness       0.31

Agent critique:

"Too bright and insufficiently rough."

Mutation:

FM index       +22%
LP cutoff      -18%
```

Damit wird aus einem Knowledge/RAG-System langfristig ein echter **AI Sound Design Agent**.

---

# 38. Nicht in Version 1 bauen

Folgende Dinge bewusst zurückstellen:

- komplexe Multi-Agent-Systeme
- vollständige DSP-Synthese
- automatische Audio-Generierung
- eigene Foundation Models
- riesiger Knowledge Graph
- Multi-User-SaaS
- Hardware-Steuerung
- VCV-Rack-Kompatibilität im ersten Sprint

Zuerst:

```text
Knowledge
+
Evidence
+
MCP
+
Synth Domain
+
ein Agent
```

---

# 39. Priorisierte MCP-Oberfläche

## Must Have

```text
search_synth_knowledge
explain_synthesis_concept
find_techniques_for_sound
find_patch_examples

suggest_modules
create_patch
validate_patch
analyze_patch
```

## Should Have

```text
compare_techniques
get_module_definition
get_frequency_reference
get_sound_characteristics
```

## Later

```text
render_patch
analyze_audio
optimize_patch
mutate_patch
export_vcv_rack
```

---

# 40. Erfolgskriterien

Das System ist MVP-fertig, wenn ein LLM:

1. eine natürliche Soundbeschreibung versteht
2. selbstständig relevantes Synthesizerwissen über MCP recherchiert
3. Quellen/Evidence erhält
4. Syntheseverfahren vergleichen kann
5. geeignete Module auswählen kann
6. einen strukturierten Patch erzeugen kann
7. den Patch technisch validieren kann
8. Frequenz-/Harmonikinformationen erhält
9. seine eigene Konfiguration begründen kann
10. den Patch auf Basis von Analyse und Kritik verbessern kann

---

# 41. Endzustand

Das eigentliche Produkt ist nicht:

> „Ein LLM, das Synth-Presets generiert.“

Sondern:

> **Eine maschinenlesbare Synthesizer-Wissens- und Engineering-Plattform, die über MCP jedem LLM fundiertes Synthese-Wissen, Quellen, Konzepte, Module, Patch-Strukturen und Analysewerkzeuge zur Verfügung stellt.**

Das LLM wird dadurch zum kreativen Layer:

```text
                 HUMAN
                   │
                   ▼
             Natural Language
                   │
                   ▼
             ┌────────────┐
             │    LLM     │
             │ Creativity │
             │ Reasoning  │
             └─────┬──────┘
                   │
                  MCP
                   │
        ┌──────────┴───────────┐
        │                      │
        ▼                      ▼
   KNOWLEDGE                SYNTH ENGINE
        │                      │
        ├─ Books               ├─ Modules
        ├─ Wikis               ├─ Signals
        ├─ Manuals             ├─ Patch Graph
        ├─ Articles            ├─ Validation
        ├─ Papers              └─ Analysis
        └─ Evidence
```

Der wichtigste Architekturentscheid lautet daher:

**Knowledge MCP und Synth Engine MCP-Funktionen leben in einem Server, aber ihre Domain- und Anwendungsschichten bleiben getrennt.**

So kann das Projekt später sowohl als **Wissensserver für beliebige LLMs** als auch als Grundlage für einen **hochwertigen autonomen Sound-Design-Agenten** dienen.
