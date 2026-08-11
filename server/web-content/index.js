(function () {
    const textEl = document.getElementById('txt_input');
    const voiceEl = document.getElementById('voice_select');
    const speedEl = document.getElementById('speed_range');
    const speedValEl = document.getElementById('speed_value');
    const tokenEl = document.getElementById('api_token');
    const btnRun = document.getElementById('btn_run');
    const statusEl = document.getElementById('status');
    const playerWrap = document.getElementById('player_wrap');
    const audioEl = document.getElementById('audio_out');
    const btnDownload = document.getElementById('btn_download');

    const TOKEN_KEY = 'kokoro_api_token';
    const SAMPLE_RATE = 24000;

    let lastBlob = null;
    let busy = false;

    function setStatus(msg, kind) {
        statusEl.textContent = msg || '';
        statusEl.className = kind || '';
    }

    function setBusy(on) {
        busy = on;
        btnRun.disabled = on || voiceEl.disabled;
        btnRun.textContent = on ? 'Синтез…' : 'Синтезировать';
    }

    function authHeaders() {
        const token = tokenEl.value.trim();
        if (!token) return {};
        return { Authorization: 'Bearer ' + token };
    }

    function buildWavBlob(pcmBytes) {
        const numChannels = 1;
        const bitsPerSample = 16;
        const blockAlign = numChannels * bitsPerSample / 8;
        const byteRate = SAMPLE_RATE * blockAlign;
        const dataSize = pcmBytes.byteLength;
        const buffer = new ArrayBuffer(44 + dataSize);
        const view = new DataView(buffer);

        function writeStr(offset, str) {
            for (let i = 0; i < str.length; i++) {
                view.setUint8(offset + i, str.charCodeAt(i));
            }
        }

        writeStr(0, 'RIFF');
        view.setUint32(4, 36 + dataSize, true);
        writeStr(8, 'WAVE');
        writeStr(12, 'fmt ');
        view.setUint32(16, 16, true);
        view.setUint16(20, 1, true);
        view.setUint16(22, numChannels, true);
        view.setUint32(24, SAMPLE_RATE, true);
        view.setUint32(28, byteRate, true);
        view.setUint16(32, blockAlign, true);
        view.setUint16(34, bitsPerSample, true);
        writeStr(36, 'data');
        view.setUint32(40, dataSize, true);
        new Uint8Array(buffer, 44).set(pcmBytes);

        return new Blob([buffer], { type: 'audio/wav' });
    }

    function streamUrl() {
        const protocol = location.protocol === 'https:' ? 'wss:' : 'ws:';
        let url = protocol + '//' + location.host + '/api/v1/stream';
        const token = tokenEl.value.trim();
        if (token) {
            url += '?token=' + encodeURIComponent(token);
        }
        return url;
    }

    speedEl.addEventListener('input', function () {
        speedValEl.textContent = Number(speedEl.value).toFixed(2);
    });

    tokenEl.addEventListener('change', function () {
        localStorage.setItem(TOKEN_KEY, tokenEl.value.trim());
    });

    const saved = localStorage.getItem(TOKEN_KEY);
    if (saved) tokenEl.value = saved;

    async function loadVoices() {
        try {
            const res = await fetch('/api/v1/voices', { headers: authHeaders() });
            if (!res.ok) throw new Error('HTTP ' + res.status);
            const voices = await res.json();
            voiceEl.innerHTML = '';
            if (!Array.isArray(voices) || voices.length === 0) {
                voiceEl.innerHTML = '<option value="">(нет голосов)</option>';
                setStatus('Сервер не вернул голоса — проверьте модели', 'error');
                return;
            }
            for (const name of voices) {
                const opt = document.createElement('option');
                opt.value = name;
                opt.textContent = name;
                voiceEl.appendChild(opt);
            }
            voiceEl.disabled = false;
            btnRun.disabled = false;
            setStatus('Готово · ' + voices.length + ' голос(ов)', 'ok');
        } catch (err) {
            voiceEl.innerHTML = '<option value="">ошибка</option>';
            setStatus('Не удалось загрузить голоса: ' + err.message, 'error');
        }
    }

    function synthesizeViaWebSocket(body) {
        return new Promise(function (resolve, reject) {
            const pcmChunks = [];
            let finished = false;
            let failed = false;
            const ws = new WebSocket(streamUrl());
            ws.binaryType = 'arraybuffer';

            function fail(err) {
                if (failed || finished) return;
                failed = true;
                try { ws.close(); } catch (_) { /* ignore */ }
                reject(err);
            }

            ws.onopen = function () {
                ws.send(JSON.stringify(body));
            };

            ws.onmessage = function (ev) {
                if (typeof ev.data === 'string') {
                    try {
                        const msg = JSON.parse(ev.data);
                        if (msg.status === 'ok' && msg.message === 'finished') {
                            finished = true;
                            ws.close();
                            resolve(pcmChunks);
                        } else if (msg.status === 'failed') {
                            fail(new Error(msg.message || 'synthesis failed'));
                        }
                    } catch (parseErr) {
                        fail(parseErr);
                    }
                } else {
                    pcmChunks.push(new Uint8Array(ev.data));
                }
            };

            ws.onerror = function () {
                fail(new Error('WebSocket error'));
            };

            ws.onclose = function (ev) {
                if (!finished && !failed) {
                    fail(new Error(ev.reason || 'WebSocket closed'));
                }
            };
        });
    }

    async function synthesize() {
        const text = textEl.value.trim();
        if (!text) {
            setStatus('Введите текст', 'error');
            textEl.focus();
            return;
        }
        if (!voiceEl.value) {
            setStatus('Выберите голос', 'error');
            return;
        }
        if (busy) return;

        setBusy(true);
        setStatus('Синтез…');
        playerWrap.classList.remove('visible');

        if (audioEl.src && audioEl.src.startsWith('blob:')) {
            URL.revokeObjectURL(audioEl.src);
            audioEl.removeAttribute('src');
        }
        lastBlob = null;

        const body = {
            text: text,
            voice: voiceEl.value,
            speed: Number(speedEl.value),
            audio_format: 'pcm',
        };

        try {
            const pcmChunks = await synthesizeViaWebSocket(body);
            const totalBytes = pcmChunks.reduce(function (sum, c) { return sum + c.length; }, 0);
            const pcm = new Uint8Array(totalBytes);
            let offset = 0;
            for (const chunk of pcmChunks) {
                pcm.set(chunk, offset);
                offset += chunk.length;
            }
            lastBlob = buildWavBlob(pcm);
            const url = URL.createObjectURL(lastBlob);
            audioEl.src = url;
            playerWrap.classList.add('visible');
            setStatus('Готово', 'ok');
            try {
                await audioEl.play();
            } catch (_) {
                /* autoplay blocked */
            }
        } catch (err) {
            setStatus('Ошибка: ' + err.message, 'error');
        } finally {
            setBusy(false);
        }
    }

    btnRun.addEventListener('click', synthesize);

    btnDownload.addEventListener('click', function () {
        if (!lastBlob) return;
        const a = document.createElement('a');
        a.href = URL.createObjectURL(lastBlob);
        a.download = 'kokoro-' + voiceEl.value + '.wav';
        a.click();
        setTimeout(function () { URL.revokeObjectURL(a.href); }, 1000);
    });

    textEl.addEventListener('keydown', function (e) {
        if ((e.ctrlKey || e.metaKey) && e.key === 'Enter') {
            e.preventDefault();
            synthesize();
        }
    });

    loadVoices();
})();
