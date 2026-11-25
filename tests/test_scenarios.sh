#!/bin/bash

set -u

LOG_FILE="test_watchdog.log"
WATCHDOG="./watchdog"

echo "=== 1. Derleme Kontrolu ==="
make clean && make
if [ $? -ne 0 ]; then
    echo "[HATA] Derleme basarisiz!"
    exit 1
fi

rm -f "$LOG_FILE"

echo ""
echo "=== 2. Normal Calisma Testi ==="
$WATCHDOG "$LOG_FILE" &
WD_PID=$!
sleep 2

if ! kill -0 "$WD_PID" 2>/dev/null; then
    echo "[HATA] Watchdog baslatilamadi!"
    exit 1
fi
echo "[OK] Watchdog calisiyor (PID: $WD_PID)"

# Alt surecleri listele
CHILDREN=$(pgrep -P "$WD_PID")
echo "Baslatilan alt surec PID'leri: $CHILDREN"

echo ""
echo "=== 3. SIGUSR1 Durum Raporlama Testi ==="
kill -SIGUSR1 "$WD_PID"
sleep 1
echo "[OK] SIGUSR1 gonderildi."

echo ""
echo "=== 4. Beklenmedik Sonlanma ve Restart Limiti Testi (4x SIGKILL) ==="
for i in 1 2 3 4; do
    FIRST_CHILD=$(pgrep -P "$WD_PID" | head -n 1)
    if [ -n "$FIRST_CHILD" ]; then
        echo "Deneme $i: Alt surec (PID:$FIRST_CHILD) SIGKILL ile sonlandiriliyor..."
        kill -SIGKILL "$FIRST_CHILD"
        sleep 2
    else
        echo "Deneme $i: Kapatilacak aktif alt surec kalmadi."
        break
    fi
done

echo ""
echo "=== 5. SIGTERM ile Guvenli Kapanis Testi ==="
kill -SIGTERM "$WD_PID" 
wait "$WD_PID"
echo "[OK] Watchdog guvenli sekilde kapandi."

echo ""
echo "=== 6. Log Dosyasi Dogrulama ($LOG_FILE) ==="
if [ -f "$LOG_FILE" ]; then
    cat "$LOG_FILE"
else
    echo "[HATA] Log dosyasi olusturulamadi!"
    exit 1
fi

echo ""
echo "Not: Ctrl+C (SIGINT) davranisini ayrica terminalde './watchdog' calistirip elle test ediniz."