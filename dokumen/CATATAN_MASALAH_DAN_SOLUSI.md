# Catatan Temuan Masalah & Solusi Arsitektur (GPU Mind Mission Control)

Dokumen ini mencatat akar penyebab masalah (*root causes*), gejala (*symptoms*), dan solusi definitif (*fixes*) yang telah diimplementasikan pada sistem simulasi GPU Mind & Web Dashboard.

---

## 1. Masalah: Data Telemetry Membeku (UI Freeze & Diam di Tempat)

### Gejala:
- Angka pada web UI (`Total Flights`, `Max Altitude`, `Sisa Jarak Ke Bulan`, `Registers`) tidak bertambah dan tampak *hardcoded*.
- Jam/timer desktop berjalan, namun visualisasi dan metrik simulasi diam.

### Akar Penyebab (*Root Causes*):
1. **Critical Section Lock Starvation di Backend C++**:
   - Fungsi `run_mass_parallel_simulation()` di `main.cpp` sebelumnya menahan mutex (`EnterCriticalSection(&g_cs)`) terlalu lama di seluruh siklus transfer `cudaMemcpy` dan mutasi genetik DNA (10.240 agen).
   - Thread HTTP Server (`http_server_thread`) yang melayani request `GET /telemetry` mengalami *starvation/deadlock*, sehingga tidak dapat merespons data JSON ke browser.
2. **Koneksi Soket WinSock Menggantung (*Stalled Socket*)**:
   - HTTP server tidak mengirimkan header `Connection: close` dan tidak melakukan `shutdown(client_fd, SD_BOTH)`, menyebabkan soket HTTP browser tertahan dalam status `TIME_WAIT` / `SYN_SENT`.
3. **Pencampuran Loop Fetch & Render di Frontend (Async Loop Lock)**:
   - Pemanggilan `await fetch('/telemetry')` diletakkan langsung di dalam loop `requestAnimationFrame(updateLoop)`. Begitu ada 1 fetch yang tertunda di jaringan, seluruh pipeline rendering Canvas 60 FPS ikut terhenti total.
4. **JavaScript ReferenceError pada Canvas Animation**:
   - Variabel `trail` pada `web/index.html` digunakan di dalam `renderCanvas()` namun belum dideklarasikan di scope manapun, memicu silent exception `ReferenceError: trail is not defined` yang menghentikan eksekusi script selanjutnya.

### Solusi & Standar Implementasi:
1. **Minimalisir Mutex Scope di C++ (`main.cpp`)**:
   - Mutex `g_cs` hanya di-*lock* saat meng-copy metrik global yang diperlukan (`g_best_agent_idx`, `g_total_global_flights`, `g_best_landing_formula`), di luar proses `cudaMemcpy` dan seleksi genetik.
2. **Protokol HTTP Soket Ringan yang Bersih**:
   - Ditambahkan `setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR)` dan header `Connection: close` serta `shutdown(client_fd, SD_BOTH)` sebelum `closesocket()`.
3. **Pemisahan Pipeline Render & Network di Browser**:
   - **Render Loop**: Berjalan murni 60 FPS via `requestAnimationFrame(updateUI)`.
   - **Data Fetcher**: Berjalan independen via `setInterval(fetchTelemetry, 50)` dengan proteksi `AbortController(timeout: 300ms)`.
4. **Inisialisasi Variabel Global**:
   - Inisialisasi eksplisit `const trail = [];` pada scope utama script.

---

## 2. Masalah: Objek Bumi & Bulan Hilang Saat Jendela di-Maximize

### Gejala:
- Saat window Edge / browser di-*maximize* ke layar penuh, planet Bumi dan Bulan menghilang dari canvas.

### Akar Penyebab:
- Ukuran CSS layout (`width: 100%`) membesar secara dinamis, namun ukuran resolusi bitmap internal (`canvas.width` dan `canvas.height`) tidak ikut disinkronkan.
- Perhitungan titik koordinat $Y$ pusat (`startY = canvas.height / 2`) menggunakan nilai tinggi lama, sehingga elemen terlempar ke luar area pandang (*off-screen*).

### Solusi:
- Di fungsi `renderCanvas()`, panggil `resizeCanvas()` setiap frame untuk membaca `canvas.getBoundingClientRect()` dan langsung memperbarui `canvas.width` & `canvas.height` jika terjadi perubahan ukuran jendela.

---

## 3. Validasi & Verifikasi Hasil

| Verifikasi | Status | Hasil |
| :--- | :--- | :--- |
| **Penerbangan T=5s** | Lulus | 787.968 flights |
| **Penerbangan T=20s** | Lulus | 806.400 flights (+18.432 fl) |
| **Penerbangan T=30s** | Lulus | 822.784 flights (+16.384 fl) |
| **Animasi Canvas** | Lulus | Partikel swarm dan gelombang armada roket bergerak halus 60 FPS |
| **Jarak ke Bulan** | Lulus | Berkurang secara dinamis mendekati target pendaratan |
