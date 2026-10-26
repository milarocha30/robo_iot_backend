import sqlite3
from contextlib import closing

from flask import Flask, jsonify, request

DB_PATH = "sensores.db"
CAMPOS = [
    "timestamp",
    "temperatura_c",
    "umidade_pct",
    "luminosidade",
    "presenca",
    "probabilidade_vida",
]

app = Flask(__name__)


def get_conn():
    conn = sqlite3.connect(DB_PATH)
    conn.row_factory = sqlite3.Row
    return conn


def init_db():
    with closing(get_conn()) as conn:
        conn.execute(
            """
            CREATE TABLE IF NOT EXISTS leituras (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                timestamp TEXT NOT NULL,
                temperatura_c REAL,
                umidade_pct REAL,
                luminosidade INTEGER,
                presenca INTEGER,
                probabilidade_vida REAL
            )
            """
        )
        conn.commit()


@app.get("/")
def inicio():
    return jsonify({"status": "online", "rotas": ["GET /leituras", "POST /leituras"]})


@app.post("/leituras")
def criar_leitura():
    dados = request.get_json(silent=True)
    if not isinstance(dados, dict):
        return jsonify({"erro": "JSON inválido"}), 400

    faltando = [c for c in CAMPOS if c not in dados]
    if faltando:
        return jsonify({"erro": "Campos ausentes", "campos": faltando}), 400

    with closing(get_conn()) as conn:
        cur = conn.execute(
            """
            INSERT INTO leituras
              (timestamp, temperatura_c, umidade_pct, luminosidade, presenca, probabilidade_vida)
            VALUES (?, ?, ?, ?, ?, ?)
            """,
            [dados[c] for c in CAMPOS],
        )
        conn.commit()
        novo_id = cur.lastrowid

    return jsonify({"status": "ok", "id": novo_id}), 201


@app.get("/leituras")
def listar_leituras():
    with closing(get_conn()) as conn:
        linhas = conn.execute(
            "SELECT * FROM leituras ORDER BY id DESC LIMIT 100"
        ).fetchall()
    return jsonify([dict(l) for l in linhas])


if __name__ == "__main__":
    init_db()
    app.run(host="0.0.0.0", port=8000, debug=False)