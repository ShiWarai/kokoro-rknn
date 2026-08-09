(function () {
    const textEl = document.getElementById('txt_input');
    const voiceEl = document.getElementById('voice_select');
    const speedEl = document.getElementById('speed_range');
    const speedValEl = document.getElementById('speed_value');
    const btnRun = document.getElementById('btn_run');
    const statusEl = document.getElementById('status');
    const playerWrap = document.getElementById('player_wrap');
    const audioEl = document.getElementById('audio_out');
    const btnDownload = document.getElementById('btn_download');

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

    speedEl.addEventListener('input', function () {
        speedValEl.textContent = Number(speedEl.value).toFixed(2);
    });

    async function loadVoices() {
        try {
            const res = await fetch('/api/v1/voices');
            if (!res.ok) throw new Error('HTTP ' + res.status);
            const voices = await res.json();
            voiceEl.innerHTML = '';
            if (!Array.isArray(voices) || voices.length === 0) {
                voiceEl.innerHTML = '<option value="">(нет голосов)</option>';
                setStatus('Сервер не вернул голоса — проверьте --voices-dir', 'error');
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
            audio_format: 'wav',
        };

        try {
            const res = await fetch('/api/v1/synthesise', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify(body),
            });
            if (!res.ok) {
                const errText = await res.text();
                throw new Error(errText || ('HTTP ' + res.status));
            }
            lastBlob = await res.blob();
            const url = URL.createObjectURL(lastBlob);
            audioEl.src = url;
            playerWrap.classList.add('visible');
            setStatus('Готово', 'ok');
            try {
                await audioEl.play();
            } catch (_) {
                /* autoplay blocked — user can press play */
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
