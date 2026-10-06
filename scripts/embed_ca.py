# Pré-build PlatformIO : génère include/MqttCa.h à partir du CA du broker MQTT.
# Source : $SENTINEL_CA, sinon secrets/ca.crt du dépôt parent (main/secrets/ca.crt).
# Le fichier généré n'est pas commité (.gitignore).
import os
from pathlib import Path

Import("env")  # noqa: F821 (fourni par SCons)

project = Path(env.subst("$PROJECT_DIR"))  # noqa: F821
source = Path(os.environ.get("SENTINEL_CA") or project.parent.parent / "secrets" / "ca.crt")
output = project / "include" / "MqttCa.h"

if source.is_file():
    pem = source.read_text().strip()
    if "BEGIN CERTIFICATE" not in pem or "PRIVATE KEY" in pem:
        print(f"embed_ca: {source} n'est pas un certificat PEM")
        env.Exit(1)  # noqa: F821
    content = (
        "#pragma once\n"
        "// Généré par scripts/embed_ca.py depuis secrets/ca.crt : ne pas éditer ni commiter.\n"
        "#include <pgmspace.h>\n\n"
        f'static const char MQTT_CA_CERT[] PROGMEM = R"EOF(\n{pem}\n)EOF";\n'
    )
    if not output.is_file() or output.read_text() != content:
        output.write_text(content)
        print(f"embed_ca: {output.name} mis à jour depuis {source}")
elif not output.is_file():
    print(f"embed_ca: CA introuvable ({source}). Placer ca.crt dans main/secrets/ "
          "ou définir SENTINEL_CA=/chemin/ca.crt")
    env.Exit(1)  # noqa: F821
