#define _GNU_SOURCE
/**
 * server.c — Serveur HTTP minimal pour Discrete Geometry Studio
 *
 * Compilation :
 *   gcc -o server server.c -lpthread
 *
 * Usage :
 *   ./server [port]        (défaut : 8080)
 *
 * Prérequis :
 *   - Le binaire ./geometrie doit être dans le même dossier
 *   - Le dossier ./web_output/ sera créé automatiquement
 *
 * Ouvrir dans le navigateur : http://localhost:8080
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <errno.h>
#include <time.h>


#define BUFSIZE      (4 * 1024 * 1024) /* 4 Mo max pour les requêtes */
#define RESPONSE_MAX (8 * 1024 * 1024) /* 8 Mo max pour les réponses */
#define MAX_PGM      (2 * 1024 * 1024) /* 2 Mo max par image PGM */

static int server_fd = -1;

/* ─── Utilitaires ─────────────────────────────────────────────── */

static void ensure_dir(const char *path) {
    struct stat st;
    if (stat(path, &st) == -1) {
        mkdir(path, 0755);
    }
}

/* Encoder en base64 */
static const char b64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static char *base64_encode(const unsigned char *data, long len, long *out_len) {
    long olen = 4 * ((len + 2) / 3);
    char *out = malloc(olen + 1);
    if (!out) return NULL;
    long j = 0;
    for (long i = 0; i < len; i += 3) {
        unsigned int v = ((unsigned int)data[i]) << 16;
        if (i + 1 < len) v |= ((unsigned int)data[i + 1]) << 8;
        if (i + 2 < len) v |= data[i + 2];
        out[j++] = b64[(v >> 18) & 0x3F];
        out[j++] = b64[(v >> 12) & 0x3F];
        out[j++] = (i + 1 < len) ? b64[(v >> 6) & 0x3F] : '=';
        out[j++] = (i + 2 < len) ? b64[v & 0x3F] : '=';
    }
    out[j] = 0;
    *out_len = j;
    return out;
}

/* Convertir PGM (P5) ou PPM (P6) en BMP 24-bit pour le navigateur */
static unsigned char *image_to_bmp(const char *path, long *bmp_size) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;

    char magic[3];
    if (fscanf(f, "%2s", magic) != 1) { fclose(f); return NULL; }

    int is_ppm = 0; /* 0 = PGM (P5), 1 = PPM (P6) */
    if (strcmp(magic, "P6") == 0) is_ppm = 1;
    else if (strcmp(magic, "P5") != 0) { fclose(f); return NULL; }

    int w = 0, h = 0, maxv = 0;
    char line[256];
    int vals = 0;
    while (vals < 3) {
        if (!fgets(line, sizeof(line), f)) { fclose(f); return NULL; }
        if (line[0] == '#') continue;
        char *p = line;
        while (vals < 3) {
            while (*p && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')) p++;
            if (!*p || *p == '#') break;
            int v, n;
            if (sscanf(p, "%d%n", &v, &n) == 1) {
                if (vals == 0) w = v;
                else if (vals == 1) h = v;
                else maxv = v;
                vals++;
                p += n;
            } else break;
        }
    }

    if (w <= 0 || h <= 0 || maxv <= 0) { fclose(f); return NULL; }

    /* Lire les pixels */
    int channels = is_ppm ? 3 : 1;
    long pixel_count = (long)w * h * channels;
    unsigned char *pixels = malloc(pixel_count);
    if (!pixels) { fclose(f); return NULL; }

    if (fread(pixels, 1, pixel_count, f) != (size_t)pixel_count) {
        free(pixels);
        fclose(f);
        return NULL;
    }
    fclose(f);

    /* Construire BMP 24-bit */
    int row_size = ((w * 3 + 3) / 4) * 4;
    int data_size = row_size * h;
    int file_size = 54 + data_size;
    unsigned char *bmp = calloc(file_size, 1);
    if (!bmp) { free(pixels); return NULL; }

    /* Header BMP */
    bmp[0] = 'B'; bmp[1] = 'M';
    bmp[2] = file_size & 0xFF; bmp[3] = (file_size >> 8) & 0xFF;
    bmp[4] = (file_size >> 16) & 0xFF; bmp[5] = (file_size >> 24) & 0xFF;
    bmp[10] = 54;

    /* DIB header */
    bmp[14] = 40;
    bmp[18] = w & 0xFF; bmp[19] = (w >> 8) & 0xFF;
    bmp[20] = (w >> 16) & 0xFF; bmp[21] = (w >> 24) & 0xFF;
    bmp[22] = h & 0xFF; bmp[23] = (h >> 8) & 0xFF;
    bmp[24] = (h >> 16) & 0xFF; bmp[25] = (h >> 24) & 0xFF;
    bmp[26] = 1;
    bmp[28] = 24;
    bmp[34] = data_size & 0xFF; bmp[35] = (data_size >> 8) & 0xFF;
    bmp[36] = (data_size >> 16) & 0xFF; bmp[37] = (data_size >> 24) & 0xFF;

    /* Pixel data (BMP est bottom-up, BGR) */
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int src_idx = ((h - 1 - y) * w + x) * channels;
            int dst_idx = 54 + y * row_size + x * 3;
            if (is_ppm) {
                /* PPM = RGB → BMP = BGR */
                bmp[dst_idx]     = pixels[src_idx + 2]; /* B */
                bmp[dst_idx + 1] = pixels[src_idx + 1]; /* G */
                bmp[dst_idx + 2] = pixels[src_idx];     /* R */
            } else {
                /* PGM = gris → BMP = gris sur 3 canaux */
                unsigned char v = pixels[src_idx];
                bmp[dst_idx]     = v; /* B */
                bmp[dst_idx + 1] = v; /* G */
                bmp[dst_idx + 2] = v; /* R */
            }
        }
    }

    free(pixels);
    *bmp_size = file_size;
    return bmp;
}


/* ─── Page HTML intégrée ──────────────────────────────────────── */
static const char *HTML_PAGE =
"<!DOCTYPE html>\n"
"<html lang='fr'>\n"
"<head>\n"
"<meta charset='UTF-8'>\n"
"<meta name='viewport' content='width=device-width,initial-scale=1'>\n"
"<title>Discrete Geometry Studio</title>\n"
"<style>\n"
"*{margin:0;padding:0;box-sizing:border-box}\n"
"body{font-family:system-ui,-apple-system,sans-serif;background:#0a0b0f;color:#e8e6e1;min-height:100vh}\n"
".hdr{padding:20px 28px;border-bottom:1px solid rgba(255,255,255,.06);display:flex;justify-content:space-between;align-items:center}\n"
".hdr h1{font-size:17px;font-weight:600}.hdr h1 b{color:#4facfe}\n"
".hdr small{font-size:11px;color:#9a9890;font-family:monospace}\n"
".main{padding:24px 28px;max-width:960px;margin:0 auto}\n"
".upload-zone{border:2px dashed rgba(79,172,254,.3);border-radius:10px;padding:40px;text-align:center;cursor:pointer;background:#12141a;transition:.3s;margin-bottom:20px}\n"
".upload-zone:hover{border-color:#4facfe;background:rgba(79,172,254,.04)}\n"
".upload-zone h2{font-size:15px;font-weight:500;margin-bottom:6px}\n"
".upload-zone p{font-size:12px;color:#9a9890}\n"
"#status{display:none;background:#12141a;border:1px solid rgba(255,255,255,.06);border-radius:10px;padding:16px 20px;margin-bottom:20px;font-size:13px}\n"
"#status.show{display:block}\n"
"#status .spinner{display:inline-block;width:14px;height:14px;border:2px solid #4facfe;border-top-color:transparent;border-radius:50%;animation:spin .8s linear infinite;margin-right:8px;vertical-align:middle}\n"
"@keyframes spin{to{transform:rotate(360deg)}}\n"
".params{display:flex;gap:12px;margin-bottom:20px;flex-wrap:wrap}\n"
".params label{font-size:12px;color:#9a9890}\n"
".params input,.params select{background:#1a1d26;border:1px solid rgba(255,255,255,.1);color:#e8e6e1;padding:6px 10px;border-radius:6px;font-size:13px;font-family:inherit;width:120px}\n"
".params button{background:#4facfe;color:#000;border:none;padding:8px 20px;border-radius:6px;font-size:13px;font-weight:600;cursor:pointer;transition:.2s}\n"
".params button:hover{background:#6fbfff}\n"
".params button:disabled{opacity:.4;cursor:not-allowed}\n"
".grid{display:grid;grid-template-columns:repeat(3,1fr);gap:14px;margin-bottom:20px}\n"
"@media(max-width:700px){.grid{grid-template-columns:repeat(2,1fr)}}\n"
".card{background:#12141a;border:1px solid rgba(255,255,255,.06);border-radius:10px;overflow:hidden;cursor:pointer;transition:.2s}\n"
".card:hover{transform:translateY(-2px);border-color:rgba(255,255,255,.12)}\n"
".card img{width:100%;aspect-ratio:1;object-fit:contain;display:block;background:#000;image-rendering:pixelated}\n"
".card-info{padding:8px 10px}\n"
".card-step{font-size:9px;font-weight:600;text-transform:uppercase;letter-spacing:1px;margin-bottom:1px}\n"
".card-title{font-size:12px;font-weight:500}\n"
".card-sub{font-size:10px;color:#9a9890;margin-top:1px}\n"
"#zoom{display:none;background:#12141a;border:1px solid rgba(255,255,255,.06);border-radius:10px;overflow:hidden;margin-bottom:20px}\n"
"#zoom.show{display:block}\n"
"#zoom .zh{padding:10px 14px;border-bottom:1px solid rgba(255,255,255,.06);font-size:13px;font-weight:500}\n"
"#zoom img{width:100%;max-height:500px;object-fit:contain;display:block;image-rendering:pixelated;background:#000}\n"
".stats{display:none;grid-template-columns:repeat(4,1fr);gap:10px;margin-bottom:20px}\n"
".stats.show{display:grid}\n"
".sc{background:#12141a;border:1px solid rgba(255,255,255,.06);border-radius:8px;padding:10px 12px}\n"
".sc .sl{font-size:10px;color:#9a9890}.sc .sv{font-size:18px;font-weight:600;margin-top:2px}\n"
".sv.b{color:#4facfe}.sv.g{color:#51cf66}.sv.r{color:#ff6b6b}.sv.y{color:#ffd43b}\n"
".log{display:none;background:#0d0e12;border:1px solid rgba(255,255,255,.06);border-radius:10px;padding:14px 16px;margin-bottom:20px;font-family:monospace;font-size:11px;color:#9a9890;white-space:pre-wrap;max-height:300px;overflow-y:auto}\n"
".log.show{display:block}\n"
"</style>\n"
"</head>\n"
"<body>\n"
"<div class='hdr'><div><h1><b>&#9670;</b> Discrete Geometry Studio</h1></div><small>Serveur C local</small></div>\n"
"<div class='main'>\n"
"\n"
"<div class='upload-zone' id='dropZone' onclick='document.getElementById(\"fInput\").click()'>\n"
"  <h2>Deposez une image PGM / PPM</h2>\n"
"  <p>ou cliquez pour selectionner</p>\n"
"  <input type='file' id='fInput' accept='.pgm,.ppm' style='display:none'>\n"
"</div>\n"
"\n"
"<div class='params'>\n"
"  <div><label>Seuil binarisation</label><br><input type='number' id='pBin' value='128' min='0' max='255'></div>\n"
"  <div><label>Seuil bissectrice (rad)</label><br><input type='number' id='pBis' value='0.7' step='0.05' min='0' max='3.14'></div>\n"
"  <button id='btnRun' disabled onclick='runPipeline()'>Lancer le pipeline</button>\n"
"</div>\n"
"\n"
"<div id='status'></div>\n"
"<div class='stats' id='stats'>\n"
"  <div class='sc'><div class='sl'>Dimensions</div><div class='sv b' id='sDim'>-</div></div>\n"
"  <div class='sc'><div class='sl'>Temps EDT</div><div class='sv g' id='sEdt'>-</div></div>\n"
"  <div class='sc'><div class='sl'>Temps bissectrice</div><div class='sv y' id='sBis'>-</div></div>\n"
"  <div class='sc'><div class='sl'>Pts axe median</div><div class='sv r' id='sMa'>-</div></div>\n"
"</div>\n"
"<div class='grid' id='grid'></div>\n"
"<div id='zoom'><div class='zh' id='zoomTitle'></div><img id='zoomImg'></div>\n"
"<div class='log' id='log'></div>\n"
"\n"
"</div>\n"
"<script>\n"
"let uploadedFile = null;\n"
"const dropZone = document.getElementById('dropZone');\n"
"const fInput = document.getElementById('fInput');\n"
"\n"
"dropZone.addEventListener('dragover', e => { e.preventDefault(); dropZone.style.borderColor='#4facfe'; });\n"
"dropZone.addEventListener('dragleave', () => { dropZone.style.borderColor=''; });\n"
"dropZone.addEventListener('drop', e => { e.preventDefault(); dropZone.style.borderColor=''; handleFile(e.dataTransfer.files[0]); });\n"
"fInput.addEventListener('change', e => { if(e.target.files[0]) handleFile(e.target.files[0]); });\n"
"\n"
"function handleFile(file) {\n"
"  if (!file.name.endsWith('.pgm') && !file.name.endsWith('.ppm')) { alert('Fichier PGM attendu'); return; }\n"
"  uploadedFile = file;\n"
"  dropZone.querySelector('h2').textContent = file.name;\n"
"  dropZone.querySelector('p').textContent = (file.size/1024).toFixed(1) + ' Ko';\n"
"  document.getElementById('btnRun').disabled = false;\n"
"}\n"
"\n"
"async function runPipeline() {\n"
"  if (!uploadedFile) return;\n"
"  const btn = document.getElementById('btnRun');\n"
"  btn.disabled = true; btn.textContent = 'Traitement...';\n"
"  const status = document.getElementById('status');\n"
"  status.innerHTML = '<span class=\"spinner\"></span> Pipeline en cours...';\n"
"  status.className = 'show';\n"
"\n"
"  const formData = new FormData();\n"
"  formData.append('image', uploadedFile);\n"
"  formData.append('seuil_bin', document.getElementById('pBin').value);\n"
"  formData.append('seuil_bis', document.getElementById('pBis').value);\n"
"\n"
"  try {\n"
"    const resp = await fetch('/api/process', { method: 'POST', body: formData });\n"
"    const data = await resp.json();\n"
"\n"
"    if (data.error) { status.innerHTML = '&#10060; Erreur : ' + data.error; btn.textContent='Lancer le pipeline'; btn.disabled=false; return; }\n"
"\n"
"    status.innerHTML = '&#9989; Pipeline termine en ' + data.total_time;\n"
"\n"
"    /* Stats */\n"
"    document.getElementById('stats').className = 'stats show';\n"
"    document.getElementById('sDim').textContent = data.width + 'x' + data.height;\n"
"    document.getElementById('sEdt').textContent = data.edt_time || '-';\n"
"    document.getElementById('sBis').textContent = data.bis_time || '-';\n"
"    document.getElementById('sMa').textContent = data.ma_points || '-';\n"
"\n"
"    /* Log */\n"
"    if (data.log) {\n"
"      document.getElementById('log').textContent = data.log;\n"
"      document.getElementById('log').className = 'log show';\n"
"    }\n"
"\n"
"    /* Images */\n"
"    const grid = document.getElementById('grid');\n"
"    grid.innerHTML = '';\n"
"    const steps = [\n"
"      {key:'binary', step:1, title:'Binarisation', sub:'Seuillage', color:'#4facfe'},\n"
"      {key:'edt', step:2, title:'Carte de distance', sub:'EDT D\\u00b2', color:'#00f2fe'},\n"
"      {key:'bisector', step:3, title:'Bissectrice', sub:'\\u03b8_X', color:'#ffd43b'},\n"
"      {key:'medial', step:4, title:'Axe median', sub:'Boules max.', color:'#ff6b6b'},\n"
"      {key:'filtered', step:5, title:'Axe filtre', sub:'Seuil \\u03b8', color:'#51cf66'},\n"
"      {key:'overlay', step:6, title:'Superposition', sub:'Vue combinee', color:'#c084fc'},\n"
"    ];\n"
"    steps.forEach(s => {\n"
"      if (!data.images[s.key]) return;\n"
"      const card = document.createElement('div');\n"
"      card.className = 'card';\n"
"      card.innerHTML = '<img src=\"data:image/bmp;base64,' + data.images[s.key] + '\">'\n"
"        + '<div class=\"card-info\">'\n"
"        + '<div class=\"card-step\" style=\"color:'+s.color+'\">Etape '+s.step+'</div>'\n"
"        + '<div class=\"card-title\">'+s.title+'</div>'\n"
"        + '<div class=\"card-sub\">'+s.sub+'</div>'\n"
"        + '</div>';\n"
"      card.onclick = () => {\n"
"        document.getElementById('zoom').className = 'show';\n"
"        document.getElementById('zoomTitle').textContent = s.title;\n"
"        document.getElementById('zoomImg').src = 'data:image/bmp;base64,' + data.images[s.key];\n"
"      };\n"
"      grid.appendChild(card);\n"
"    });\n"
"\n"
"  } catch(e) { status.innerHTML = '&#10060; Erreur reseau : ' + e.message; }\n"
"  btn.textContent = 'Lancer le pipeline'; btn.disabled = false;\n"
"}\n"
"</script>\n"
"</body></html>\n";

/* ─── Serveur HTTP ────────────────────────────────────────────── */

static void send_response(int fd, int code, const char *ctype, const char *body, long body_len) {
    char header[512];
    int hlen = snprintf(header, sizeof(header),
        "HTTP/1.1 %d OK\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %ld\r\n"
        "Connection: close\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "\r\n",
        code, ctype, body_len);

    // Correction: Vérification de la valeur de retour de write
    if (write(fd, header, hlen) != hlen) {
        close(fd);
        return;
    }

    if (body && body_len > 0) {
        long sent = 0;
        while (sent < body_len) {
            long n = write(fd, body + sent, body_len - sent);
            if (n <= 0) break;
            sent += n;
        }
    }
}

static void send_json(int fd, const char *json) {
    send_response(fd, 200, "application/json; charset=utf-8", json, strlen(json));
}

/* Extraire le corps du multipart/form-data (simplifié) */
static int extract_multipart_file(const char *req, long req_len, const char *boundary,
                                   char *out_data, long *out_len, long max_len) {
    char delim[256];
    snprintf(delim, sizeof(delim), "--%s", boundary);
    int dlen = strlen(delim);

    char *p = memmem(req, req_len, delim, dlen);
    if (!p) return 0;

    /* Chercher le fichier (Content-Disposition: ... filename=) */
    char *filepart = memmem(p, req_len - (p - req), "filename=", 9);
    if (!filepart) return 0;

    /* Trouver le début des données (double \r\n) */
    char *data_start = memmem(filepart, req_len - (filepart - req), "\r\n\r\n", 4);
    if (!data_start) return 0;
    data_start += 4;

    /* Trouver la fin (prochain boundary) */
    char *data_end = memmem(data_start, req_len - (data_start - req), delim, dlen);
    if (!data_end) return 0;
    data_end -= 2; /* retirer \r\n avant le boundary */

    long len = data_end - data_start;
    if (len > max_len) len = max_len;
    memcpy(out_data, data_start, len);
    *out_len = len;
    return 1;
}

/* Extraire un champ texte du multipart */
static int extract_multipart_field(const char *req, long req_len, const char *boundary,
                                    const char *field_name, char *out, int out_max) {
    char pattern[256];
    snprintf(pattern, sizeof(pattern), "name=\"%s\"", field_name);
    char *p = memmem(req, req_len, pattern, strlen(pattern));
    if (!p) return 0;
    char *data_start = memmem(p, req_len - (p - req), "\r\n\r\n", 4);
    if (!data_start) return 0;
    data_start += 4;
    char delim[256];
    snprintf(delim, sizeof(delim), "--%s", boundary);
    char *data_end = memmem(data_start, req_len - (data_start - req), delim, strlen(delim));
    if (!data_end) return 0;
    data_end -= 2;
    int len = data_end - data_start;
    if (len >= out_max) len = out_max - 1;
    memcpy(out, data_start, len);
    out[len] = 0;
    return 1;
}

/* Traiter la requête POST /api/process */
/* ─── Fonction d'échappement JSON (RFC 8259) ───────────────────── */
static int json_escape(char *dest, const char *src, int dest_size) {
    int written = 0;
    for (int i = 0; src[i] && written < dest_size - 1; i++) {
        switch (src[i]) {
            case '"':  written += snprintf(dest + written, dest_size - written, "\\\"");
                       break;
            case '\\': written += snprintf(dest + written, dest_size - written, "\\\\");
                       break;
            case '\b': written += snprintf(dest + written, dest_size - written, "\\b");
                       break;
            case '\f': written += snprintf(dest + written, dest_size - written, "\\f");
                       break;
            case '\n': written += snprintf(dest + written, dest_size - written, "\\n");
                       break;
            case '\r': written += snprintf(dest + written, dest_size - written, "\\r");
                       break;
            case '\t': written += snprintf(dest + written, dest_size - written, "\\t");
                       break;
            default:
                if (src[i] >= 0 && src[i] < 32) {
                    // Caractère de contrôle : échappement en \uXXXX
                    written += snprintf(dest + written, dest_size - written, "\\u%04x", src[i]);
                } else {
                    dest[written++] = src[i];
                }
                break;
        }
    }
    dest[written] = '\0';
    return written;
}

// ... [Le reste du code reste identique jusqu'à handle_process] ...

/* Traiter la requête POST /api/process */
static void handle_process(int client_fd, const char *req, long req_len, const char *boundary) {
    ensure_dir("web_output");

    /* Extraire le fichier PGM */
    char *pgm_data = malloc(MAX_PGM);
    long pgm_len = 0;
    if (!extract_multipart_file(req, req_len, boundary, pgm_data, &pgm_len, MAX_PGM)) {
        send_json(client_fd, "{\"error\":\"Fichier PGM non trouvé dans la requête\"}");
        free(pgm_data);
        return;
    }

    /* Extraire les paramètres */
    char seuil_bin[32] = "128";
    char seuil_bis[32] = "0.7";
    extract_multipart_field(req, req_len, boundary, "seuil_bin", seuil_bin, sizeof(seuil_bin));
    extract_multipart_field(req, req_len, boundary, "seuil_bis", seuil_bis, sizeof(seuil_bis));

    /* Sauver le PGM sur disque */
    const char *input_path = "web_output/input.pgm";
    FILE *f = fopen(input_path, "wb");
    if (!f) {
        send_json(client_fd, "{\"error\":\"Impossible de sauver le fichier\"}");
        free(pgm_data);
        return;
    }
    if (fwrite(pgm_data, 1, pgm_len, f) != (size_t)pgm_len) {
        fclose(f);
        free(pgm_data);
        send_json(client_fd, "{\"error\":\"Erreur lors de l'écriture du fichier\"}");
        return;
    }
    fclose(f);
    free(pgm_data);

    /* Exécuter le pipeline */
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "./geometrie %s web_output %s %s 2>&1", input_path, seuil_bin, seuil_bis);

    printf("[server] Exécution: %s\n", cmd);
    clock_t t0 = clock();

    FILE *pipe = popen(cmd, "r");
    if (!pipe) {
        send_json(client_fd, "{\"error\":\"Impossible de lancer ./geometrie\"}");
        return;
    }

    char log_buf[8192] = {0};
    int log_pos = 0;
    char line[512];
    while (fgets(line, sizeof(line), pipe)) {
        int n = strlen(line);
        if (log_pos + n < (int)sizeof(log_buf) - 1) {
            memcpy(log_buf + log_pos, line, n);
            log_pos += n;
        }
    }
    log_buf[log_pos] = 0;
    int ret = pclose(pipe);

    clock_t t1 = clock();
    double total_ms = (double)(t1 - t0) / CLOCKS_PER_SEC * 1000.0;

    if (ret != 0) {
        char escaped_log[8192 * 6];
        int escaped_len = json_escape(escaped_log, log_buf, sizeof(escaped_log));
        char err[9000];
        snprintf(err, sizeof(err), "{\"error\":\"geometrie a échoué (code %d)\",\"log\":\"%.*s\"}",
                 ret, escaped_len, escaped_log);
        send_json(client_fd, err);
        return;
    }

    /* Extraire les stats du log */
    char edt_time[32] = "-", bis_time[32] = "-", ma_points[32] = "-";
    char width_s[32] = "?", height_s[32] = "?";

    char *p;
    if ((p = strstr(log_buf, "Dimensions : ")) != NULL) {
        sscanf(p + 13, "%s x %s", width_s, height_s);
    }
    if ((p = strstr(log_buf, "Temps EDT : ")) != NULL) {
        sscanf(p + 12, "%s", edt_time);
    }
    if ((p = strstr(log_buf, "Temps bissectrice : ")) != NULL) {
        sscanf(p + 19, "%s", bis_time);
    }
    if ((p = strstr(log_buf, "Points axe")) != NULL) {
        char *colon = strchr(p, ':');
        if (colon) sscanf(colon + 1, "%s", ma_points);
    }

    /* Convertir les PGM de sortie en BMP base64 */
    const char *files[] = {
        "web_output/01_binary.pgm",
        "web_output/02_edt.pgm",
        "web_output/03_bisector.pgm",
        "web_output/04_medial_axis.pgm",
        "web_output/05_filtered_axis.pgm",
        "web_output/06_overlay.pgm",
    };
    const char *keys[] = {"binary","edt","bisector","medial","filtered","overlay"};

    /* Construire la réponse JSON */
    char *response = malloc(RESPONSE_MAX);
    if (!response) {
        send_json(client_fd, "{\"error\":\"Erreur mémoire\"}");
        return;
    }

    int rpos = snprintf(response, RESPONSE_MAX,
        "{\"width\":\"%s\",\"height\":\"%s\","
        "\"edt_time\":\"%s\",\"bis_time\":\"%s\",\"ma_points\":\"%s\","
        "\"total_time\":\"%.1f ms\","
        "\"log\":\"",
        width_s, height_s, edt_time, bis_time, ma_points, total_ms);

    /* Échappement du log pour JSON */
    char escaped_log[8192 * 6];  // Taille suffisante pour le log échappé
    int escaped_len = json_escape(escaped_log, log_buf, sizeof(escaped_log));
    if (escaped_len >= (int)sizeof(escaped_log) - 1) {
        free(response);
        send_json(client_fd, "{\"error\":\"Log trop long\"}");
        return;
    }
    memcpy(response + rpos, escaped_log, escaped_len);
    rpos += escaped_len;

    rpos += snprintf(response + rpos, RESPONSE_MAX - rpos, "\",\"images\":{");

    for (int i = 0; i < 6; i++) {
        if (rpos >= RESPONSE_MAX - 100) {
            free(response);
            send_json(client_fd, "{\"error\":\"Réponse trop grande\"}");
            return;
        }

        long bmp_size = 0;
        unsigned char *bmp = image_to_bmp(files[i], &bmp_size);
        if (!bmp) continue;

        long b64_len = 0;
        char *b64 = base64_encode(bmp, bmp_size, &b64_len);
        free(bmp);
        if (!b64) continue;

        if (i > 0) response[rpos++] = ',';
        rpos += snprintf(response + rpos, RESPONSE_MAX - rpos, "\"%s\":\"", keys[i]);

        if (rpos + b64_len >= RESPONSE_MAX - 10) {
            free(b64);
            free(response);
            send_json(client_fd, "{\"error\":\"Réponse trop grande\"}");
            return;
        }
        memcpy(response + rpos, b64, b64_len);
        rpos += b64_len;
        response[rpos++] = '"';
        free(b64);
    }

    rpos += snprintf(response + rpos, RESPONSE_MAX - rpos, "}}");
    response[rpos] = 0;

    send_json(client_fd, response);
    free(response);

    printf("[server] Pipeline terminé en %.1f ms\n", total_ms);
}
static void *handle_client(void *arg) {
    int client_fd = *(int *)arg;
    free(arg);

    char *buf = malloc(BUFSIZE);
    if (!buf) { close(client_fd); return NULL; }

    long total = 0;
    while (total < BUFSIZE - 1) {
        long n = read(client_fd, buf + total, BUFSIZE - 1 - total);
        if (n <= 0) break;
        total += n;

        /* Vérifier si on a reçu tout le contenu */
        char *hdr_end = strstr(buf, "\r\n\r\n");
        if (hdr_end) {
            char *cl = strcasestr(buf, "Content-Length:");
            if (cl) {
                long content_len = atol(cl + 15);
                long body_start = (hdr_end + 4) - buf;
                if (total - body_start >= content_len) break;
            } else {
                break;
            }
        }
    }
    buf[total] = 0;

    /* Router */
    if (strncmp(buf, "GET / ", 6) == 0 || strncmp(buf, "GET /index", 10) == 0) {
        send_response(client_fd, 200, "text/html; charset=utf-8", HTML_PAGE, strlen(HTML_PAGE));
    }
    else if (strncmp(buf, "POST /api/process", 17) == 0) {
        /* Extraire le boundary du Content-Type */
        char *ct = strcasestr(buf, "Content-Type:");
        char boundary[256] = {0};
        if (ct) {
            char *bp = strstr(ct, "boundary=");
            if (bp) {
                bp += 9;
                int i = 0;
                while (bp[i] && bp[i] != '\r' && bp[i] != '\n' && bp[i] != ' ' && i < 250)
                    { boundary[i] = bp[i]; i++; }
                boundary[i] = 0;
            }
        }
        if (boundary[0]) {
            handle_process(client_fd, buf, total, boundary);
        } else {
            send_json(client_fd, "{\"error\":\"Content-Type multipart attendu\"}");
        }
    }
    else {
        const char *msg = "{\"error\":\"Route inconnue\"}";
        send_response(client_fd, 404, "application/json", msg, strlen(msg));
    }

    free(buf);
    close(client_fd);
    return NULL;
}

static void cleanup(int sig) {
    (void)sig;
    if (server_fd >= 0) close(server_fd);
    printf("\n[server] Arret.\n");
    exit(0);
}

int main(int argc, char *argv[]) {
    int port = argc > 1 ? atoi(argv[1]) : 8080;

    signal(SIGINT, cleanup);
    signal(SIGTERM, cleanup);
    signal(SIGPIPE, SIG_IGN);

    /* Vérifier que ./geometrie existe */
    if (access("./geometrie", X_OK) != 0) {
        fprintf(stderr, "ERREUR: ./geometrie introuvable ou non executable.\n");
        fprintf(stderr, "Compilez d'abord votre projet: make\n");
        return 1;
    }

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) { perror("socket"); return 1; }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    // Ajout de SO_REUSEPORT pour les systèmes modernes
    #ifdef SO_REUSEPORT
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));
    #endif

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(port),
        .sin_addr.s_addr = INADDR_ANY,
    };

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, 10) < 0) {
        perror("listen");
        close(server_fd);
        return 1;
    }

    printf("========================================\n");
    printf("  DISCRETE GEOMETRY STUDIO — Serveur\n");
    printf("========================================\n");
    printf("  http://localhost:%d\n", port);
    printf("  Ctrl+C pour arreter\n");
    printf("========================================\n\n");

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd < 0) {
            if (errno == EINTR) continue;
            perror("accept");
            continue;
        }

        int *fd_ptr = malloc(sizeof(int));
        *fd_ptr = client_fd;
        pthread_t th;
        pthread_create(&th, NULL, handle_client, fd_ptr);
        pthread_detach(th);
    }

    close(server_fd);
    return 0;
}
