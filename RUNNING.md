# Running the Trading Engine

## Prerequisites
- C++ server built (`cmake --build build`)
- ngrok installed at `C:\ngrok\ngrok.exe`
- Frontend deployed to Vercel (automatic on every `git push`)

---

## Every time you want to go live

Open **3 terminals** and run one thing in each:

### Terminal 1 — C++ Backend
```bash
./build/trading-engine
```
You should see: `[server] listening on port 9000`

### Terminal 2 — ngrok tunnel
```powershell
cd C:\ngrok
.\ngrok http --url=overready-aggravatingly-domingo.ngrok-free.dev 9000
```
You should see: `Forwarding  https://overready-aggravatingly-domingo.ngrok-free.dev -> http://localhost:9000`

### Terminal 3 — (optional) watch logs
Leave Terminal 1 open to see live server logs as users connect and trade.

---

## Share this URL with traders
```
https://trading-engine-kdze.vercel.app/
```

## Admin login
Username: `purplepoet`

---

## Important
- Your computer must stay **on and awake** while the session is running
- If ngrok disconnects, just re-run the Terminal 2 command
- If the C++ server crashes, re-run the Terminal 1 command
- The frontend on Vercel stays up permanently — only the backend needs your machine

---

## Stopping
Just close Terminal 1 and Terminal 2. Users will be disconnected.
