@echo off
echo ===============================
echo Syncing backend JSON to frontend
echo ===============================

if not exist backend\data\beds.json (
    echo ERROR: backend\data\beds.json not found.
    pause
    exit /b
)

copy /Y backend\data\beds.json frontend\public\beds.json > nul

echo Sync complete.
