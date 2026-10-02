Die TR-808 synthetisiert ihre Sounds live – sie ist keine Sample-Drummachine.

Roland schreibt inzwischen selbst ausdrücklich, dass die hohen Kosten für Speicherchips dazu führten, dass die Entwickler analoge Synthese statt Samples verwendeten. Die Sounds wurden zunächst auf einem Roland System-700 entwickelt und anschließend mit der analogen Schaltung der TR-808 umgesetzt.

Das ist auch im originalen Service Manual konsistent: Der digitale Teil erzeugt Trigger und Instrumenten-Auswahl, danach wird der jeweilige analoge Sound Generator angestoßen. Der Service-Text nennt ausdrücklich „Sound Generators“ und beschreibt deren Schaltungen.

Also konzeptionell:

                 DIGITAL
Pattern → CPU → Common Trigger
             ↘ Instrument Select
                    │
                    ▼
              ┌───────────┐
              │ AND/Gates │
              └─────┬─────┘
                    │ Trigger
                    ▼
              ANALOG VOICE
                    │
                    ▼
                  AUDIO

Es gibt keine Drum-ROMs, aus denen die einzelnen Hits abgespielt werden.

----------------------------------

Ja — ich habe jetzt tatsächlich eine relevante Spur gefunden, aber ich würde zwei Dinge sauber trennen:

Ja — **ich habe jetzt tatsächlich eine relevante Spur gefunden**, aber ich würde zwei Dinge sauber trennen:

### Was wir sicher haben

Roland sagt offiziell, dass die 808-Sounds **zuerst auf dem System-700 entwickelt** und danach als Parameter in die wesentlich kleinere TR-808-Analogschaltung übertragen wurden. ([Roland][1])

Noch interessanter: **Tadao Kikumoto selbst** beschreibt rückblickend, dass das System-700 dabei nicht einfach ein Vorbild war, sondern dass die ursprüngliche Idee viel umfangreicher war. Laut seinem späteren Bericht war die Vorstellung, dass **jedes Instrument ungefähr die Synthesizer-Möglichkeiten eines System-700 bekommen sollte** – was wegen Kosten und Baugröße massiv reduziert werden musste. ([RC-808][2])

Das passt sehr gut zu deiner Vermutung, dass wir eigentlich nach dem **ursprünglichen Sound-Design-Prozess** suchen müssen und nicht nur nach dem finalen 808-Schaltplan.

### Und ich habe die System-700-Dokumentation gefunden

Das originale **Roland System-700 Instruction Manual von 1977** ist frei verfügbar. Es umfasst 224 Seiten und beschreibt die Module und ihre Verschaltung. ([Manuals+][3])

[Roland System-700 Instruction Manual – Scan](https://manuals.plus/m/c7067b226c9d02936d1f08f703f03a7184cb69fb847e322151fdfdb031b5f6f8?utm_source=chatgpt.com)

Darin werden u. a. die VCO-, VCF-, VCA-, Envelope-, Noise-, Ringmodulator- und Mixer-Module beschrieben. Es gibt außerdem **System-700-kompatible Patch Books** des damaligen Roland-Systems. ([Manuals+][3])

### Aber hier kommt der Haken

Ich habe **noch keinen originalen Roland-Datensatz gefunden, der sagt:**

> „TR-808 Bass Drum → System-700 Patch = VCO X → VCF Y → ADSR Z → ...“

Also genau das historische Dokument, das wir eigentlich haben wollen.

Die vorhandenen offiziellen Quellen belegen:

**System-700 → Sounddesign → TR-808-Schaltung**

aber sie veröffentlichen offenbar **nicht den ursprünglichen System-700-Patch jedes einzelnen 808-Instruments**.

Und es gibt einen sehr interessanten Grund dafür: Kikumoto beschreibt rückblickend, dass die Entwickler beim Übergang vom System-700 zur 808 **massiv vereinfachen mussten**. Die fertige 808 ist also nicht einfach ein „System-700-Preset in kleiner“. ([RC-808][2])

### Ein extrem interessanter neuer Fund

In einem Interview mit Kikumoto wird sogar erklärt, **warum** das System-700 als Ausgangspunkt nicht einfach ausreichte: Er sagt sinngemäß, dass klassische subtraktive Synthese Schwierigkeiten hat, die **chaotischen harmonischen Komponenten des Percussion-Attacks** zu erzeugen. Deshalb griff das Team auf die bereits beim CR-78 entwickelte analoge Technik zurück und modifizierte diese für die 808. ([CDM Create Digital Music][4])

Das ist für dein Modell **sehr wichtig**.

Es bedeutet offenbar:

```text
System-700
   │
   │  Syntheseversuche / Klangziel
   ▼
808-Sound-Idee
   │
   ├── „normale“ Synthese reicht nicht
   │
   ├── CR-78-Techniken
   │
   └── neue analoge Schaltungen
           │
           ▼
       TR-808 Voice
```

Also sollten wir **nicht** davon ausgehen, dass die 808-Sounds einfach klassische VCO→VCF→VCA-Patches waren.

---

### Was ich als Nächstes suchen würde

Es gibt tatsächlich noch eine ziemlich vielversprechende Spur: **System-700 Service Notes + Service Supplement + die einzelnen 101/102 Patch Books**. Die Existenz dieser Dokumente ist belegt. ([Wikipedia][5])

Wenn wir die finden und mit den **originalen 808-Schaltplänen** vergleichen, können wir möglicherweise rekonstruieren:

**„Welche System-700-Synthesetechnik wollten Kikumoto/Nakamura eigentlich erreichen → welche Teile davon wurden in die 808-Schaltung übernommen → welche wurden aus Kostengründen durch spezielle Resonator-/Noise-/Envelope-Schaltungen ersetzt?“**

Das wäre meiner Meinung nach sogar **wertvoller als ein nie gefundener „808 Patch Sheet“**, weil wir damit den tatsächlichen Entwicklungsprozess modellieren könnten.

[1]: https://www.roland.com/uk/promos/roland_tr-808/?lang=en-GB&utm_source=chatgpt.com "Roland - The TR-808 Story"
[2]: https://rc-808.com/comment/?utm_source=chatgpt.com "Comment of H. Saito - RC-808"
[3]: https://manuals.plus/m/c7067b226c9d02936d1f08f703f03a7184cb69fb847e322151fdfdb031b5f6f8?utm_source=chatgpt.com "System 700 owner's manual"
[4]: https://cdm.link/tadeo-kikumoto-808-day/?utm_source=chatgpt.com "Roland's Tadeo Kikumoto on 808, part by part: the ukiyo-e drum machine - CDM Create Digital Music"
[5]: https://en.wikipedia.org/wiki/Roland_System_700?utm_source=chatgpt.com "Roland System 700"

---

### Was ich als Nächstes suchen würde

Es gibt tatsächlich noch eine ziemlich vielversprechende Spur: **System-700 Service Notes + Service Supplement + die einzelnen 101/102 Patch Books**. Die Existenz dieser Dokumente ist belegt. ([Wikipedia][5])

Wenn wir die finden und mit den **originalen 808-Schaltplänen** vergleichen, können wir möglicherweise rekonstruieren:

**„Welche System-700-Synthesetechnik wollten Kikumoto/Nakamura eigentlich erreichen → welche Teile davon wurden in die 808-Schaltung übernommen → welche wurden aus Kostengründen durch spezielle Resonator-/Noise-/Envelope-Schaltungen ersetzt?“**

Das wäre meiner Meinung nach sogar **wertvoller als ein nie gefundener „808 Patch Sheet“**, weil wir damit den tatsächlichen Entwicklungsprozess modellieren könnten.

[1]: https://www.roland.com/uk/promos/roland_tr-808/?lang=en-GB&utm_source=chatgpt.com "Roland - The TR-808 Story"
[2]: https://rc-808.com/comment/?utm_source=chatgpt.com "Comment of H. Saito - RC-808"
[3]: https://manuals.plus/m/c7067b226c9d02936d1f08f703f03a7184cb69fb847e322151fdfdb031b5f6f8?utm_source=chatgpt.com "System 700 owner's manual"
[4]: https://cdm.link/tadeo-kikumoto-808-day/?utm_source=chatgpt.com "Roland's Tadeo Kikumoto on 808, part by part: the ukiyo-e drum machine - CDM Create Digital Music"
[5]: https://en.wikipedia.org/wiki/Roland_System_700?utm_source=chatgpt.com "Roland System 700"




https://pure.qub.ac.uk/en/publications/a-physically-informed-circuit-bendable-digital-model-of-the-rolan/?utm_source=chatgpt.com
https://www.dafx.de/paper-archive/2014/dafx14_kurt_james_werner_a_physically_informed%2C_ci.pdf?utm_source=chatgpt.com
https://pure.qub.ac.uk/files/125044847/tr_808_cymbal_a_physically_informed_circuit_bendable_digital.pdf?utm_source=chatgpt.com
https://www.researchgate.net/publication/280534006_Three_Models_of_Circuit-Bent_TR-808_Voices_the_Bass_Drum_Cymbal_and_Cowbell?utm_source=chatgpt.com
https://manuals.plus/m/c7067b226c9d02936d1f08f703f03a7184cb69fb847e322151fdfdb031b5f6f8?utm_source=chatgpt.com
https://cdm.link/tadeo-kikumoto-808-day/?utm_source=chatgpt.com
https://rc-808.com/comment/?utm_source=chatgpt.com