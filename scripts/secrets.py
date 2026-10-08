# Pré-build PlatformIO : génère include/Secrets.h (identifiants Wi-Fi et MQTT de l'ESP).
# Chaque valeur vient de la variable d'environnement du même nom, sinon du .env de la pile
# ($SENTINEL_ENV, sinon main/.env), seule source des secrets : rien n'est écrit dans le dépôt.
# Le fichier généré n'est pas commité (.gitignore).
import os
from pathlib import Path

Import("env")  # noqa: F821 (fourni par SCons)

# Constante C -> variable du .env
KEYS = {
    "WIFI_SSID": "ESP_WIFI_SSID",
    "WIFI_PASSWORD": "ESP_WIFI_PASSWORD",
    "MQTT_HOST": "ESP_MQTT_HOST",
    "MQTT_PASSWORD": "MQTT_ESP_PASSWORD",
}
PLACEHOLDERS = {"", "change-me"}

project = Path(env.subst("$PROJECT_DIR"))  # noqa: F821
env_file = Path(os.environ.get("SENTINEL_ENV") or project.parent.parent / ".env")
output = project / "include" / "Secrets.h"


def read_env(path):
    values = {}
    if not path.is_file():
        return values
    for line in path.read_text(encoding="utf-8").splitlines():
        line = line.strip()
        if not line or line.startswith("#") or "=" not in line:
            continue
        key, value = line.split("=", 1)
        value = value.strip()
        if len(value) >= 2 and value[0] == value[-1] and value[0] in "\"'":
            value = value[1:-1]
        values[key.strip()] = value
    return values


def c_string(value):
    return '"' + value.replace("\\", "\\\\").replace('"', '\\"') + '"'


dotenv = read_env(env_file)
values = {const: os.environ.get(var, dotenv.get(var, "")) for const, var in KEYS.items()}
missing = [KEYS[const] for const, value in values.items() if value in PLACEHOLDERS]
if missing:
    # Sans accent : la console Windows de PlatformIO ne les affiche pas
    print(f"secrets: valeurs manquantes dans {env_file} (ou en variable d'environnement) : {', '.join(missing)}")
    env.Exit(1)  # noqa: F821

content = (
    "#pragma once\n"
    "// Généré par scripts/secrets.py depuis le .env de la pile : ne pas éditer ni commiter.\n\n"
    + "".join(f"static const char *{const} = {c_string(value)};\n" for const, value in values.items())
)
if not output.is_file() or output.read_text(encoding="utf-8") != content:
    output.write_text(content, encoding="utf-8")
    print(f"secrets: {output.name} mis à jour depuis {env_file}")
