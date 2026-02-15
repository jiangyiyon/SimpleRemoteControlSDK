# T1004 - Quick Start Guide

## 🚀 Quick Start (5 Minutes)

### Step 1: Start Server

Run this command:
```bash
build\bin\Debug\integration_tests.exe --gtest_filter=RemoteDesktopIntegrationTest.FullStackTest
```

This will start the server for ~7 seconds with all components running.

### Step 2: Access from Desktop

Open browser and navigate to:
```
http://localhost:8080
```

### Step 3: (Optional) Test on Mobile Device

#### Get PC IP:
```bash
ipconfig
```
Look for "IPv4 Address" (e.g., 192.168.1.100)

#### Access from Mobile:
Open browser on mobile and navigate to:
```
http://<PC_IP>:8080
```

Example:
```
http://192.168.1.100:8080
```

---

## 📋 Full Testing Checklist

### Desktop Testing
- [ ] Chrome: http://localhost:8080
- [ ] Edge: http://localhost:8080
- [ ] Firefox: http://localhost:8080

### Mobile Testing
- [ ] Android: http://<PC_IP>:8080 (Chrome)
- [ ] iOS: http://<PC_IP>:8080 (Safari)

---

## 🔧 If Server Doesn't Start

### Check Ports:
```bash
netstat -ano | findstr :8080
netstat -ano | findstr :8081
```

### Configure Firewall (One-time setup):

**Option 1: GUI**
1. Windows Defender Firewall → Allow app
2. Allow `build\bin\Debug\integration_tests.exe`
3. Check both Private and Public

**Option 2: Command Line (Admin)**
```bash
netsh advfirewall firewall add rule name="TestServer" dir=in action=allow program="build\bin\Debug\integration_tests.exe"
```

---

## 📊 Testing Template

Copy and fill in:

```markdown
### Test Results for [Device] - [Browser]

**Date**: 2026-02-15
**Tester**: [Your Name]

- [ ] Page loads correctly
- [ ] Video stream appears
- [ ] Interactions work (touch/mouse/keyboard)
- [ ] Latency is acceptable (< 100ms for mobile, < 50ms for desktop)
- [ ] No significant lag or stuttering

**Notes**: [Any observations or issues]
```

---

## 📝 Test Plan & Instructions

For detailed testing procedures, see:
- **Test Plan**: `t1004_test_plan.md`
- **Build Instructions**: `t1004_build_instructions.md`

---

## 🐛 Troubleshooting

### "Connection refused"
- Check if server is running
- Check firewall settings
- Verify correct port

### "Video doesn't appear"
- Check browser console for errors
- Verify WebRTC is supported
- Try different browser

### "High latency"
- Check network speed
- Reduce video quality (adjust config)
- Try wired connection

---

**Quick Start Created**: 2026-02-15
**Status**: Ready for testing 🎯
