# Learning Wiki: Solum 🍯

Selamat datang di jurnal pembelajaran proyek **Solum**. Dokumen ini adalah "blok memori" kolektif kita—tempat kita menyimpan alasan di balik keputusan teknis, cerita dari medan tempur debugging, dan kebijaksanaan yang kita petik sepanjang jalan.

## 🏙️ Arsitektur (Peta Kota)

Solum dirancang seperti **"Jam Tangan Mekanik"**:
- **Win32 Native**: Tidak ada baterai (framework berat), tidak ada layar digital (elektron). Semuanya adalah roda gigi C++ yang presisi.
- **UI Modular**: Setiap komponen (editor, tab, scrollbar) adalah modul mandiri yang diorkestrasikan oleh `main.cpp`.
- **Renaissance Philosophy**: Menghargai detail kecil. Scrollbar yang halus, aura seleksi yang dinamis, tapi tetap ringan seperti udara.

## 🛠️ Keputusan Teknis (Alasan Mengapa)

### **Warna Aksen: Solum Amber (#EEBA2C)**
Kita memilih warna ini sebagai identitas brand yang baru. Kenapa?
- **Kontras**: Memberikan kehangatan di atas UI yang monokromatik (Akkurat Mono).
- **Format BGR**: Di dunia Win32 API, warna dibaca terbalik (Blue-Green-Red). `#EEBA2C` menjadi `0x2CBAEE`. Mengingat ini adalah "kebiasaan lama" Windows yang harus kita hormati.

## 🐛 Cerita Perang (Bug & Perbaikan)

### **Insiden "The Nuked Includes" di types.h**
Saat melakukan rebranding otomatis besar-besaran, sebuah kesalahan pada pola pencarian (multi-replace) sempat menghapus seluruh baris `#include` dan konstanta penting di `types.h`.
- **Gejala**: Kode tidak bisa dikompilasi (compiler error "unknown type name").
- **Penyebab**: String pengganti yang terlalu luas tanpa mempedulikan spasi/newline yang tepat.
- **Pelajaran**: "Measure twice, cut once." Selalu verifikasi hasil edit otomatis pada file inti sistem meskipun terlihat sepele.

## 🦉 Kebijaksanaan (Pelajaran Berharga)

### **Rebranding adalah "Jiwa", Bukan Sekadar "Teks"**
Mengubah nama proyek bukan hanya `Search & Replace`. Kita harus menyentuh:
- **Metadata Biner**: Agar Windows melihat vendor yang benar di Task Manager.
- **Data Path**: Agar user tidak kehilangan data, atau—dalam kasus kita—memulai lembaran baru yang bersih.
- **Headers**: Menghormati identitas baru di setiap baris komentar kode.

## 💎 Praktik Terbaik

- **Linguistic Consistency**: Semua string yang dilihat user **WAJIB** ada di `src/lang/`. Jangan pernah menulis L"Teks" langsung di logika UI.
- **Low-Level, High-Aesthetic**: Boleh menggunakan API rendah (Win32), tapi hasilnya harus terasa premium (GDI+/Direct2D).

---
*Dibuat pada: 14 April 2026*

## Log Pembaruan: 15 April 2026 (The Renaissance Pulse)

Sesi malam ini adalah tentang **"Stabilitas & Identitas"**. Kita mencoba mendorong batas visual Solum ke ekstremitas baru, belajar dari kegagalan, dan menyiapkan panggung untuk identitas brand yang lebih kuat.

### 🏙️ Arsitektur: PremiumOrchestrator & Spring Physics
Kita telah memperkenalkan `PremiumOrchestrator` sebagai konduktor utama untuk efek visual tinggi. Selain itu, **Spring Physics** (Second-Order Dynamics) sekarang menjadi bagian dari DNA animasi Solum, memungkinkan gerakan UI yang memiliki massa dan momentum, bukan hanya durasi statis.

### 🐛 Cerita Perang: "The Black Screen of Aura"
Kita mencoba mengubah `SelectionAura` menjadi jendela `WS_POPUP` agar bisa mendapatkan transparansi *pixel-perfect* di atas Editor.
- **Gejala**: Layar Editor menjadi hitam pekat saat Aura dipicu.
- **Penyebab**: Konflik antara context rendering Direct2D jendela induk dan `WS_EX_LAYERED` pada jendela anak. Win32 "menarik" area tersebut ke luar dari pipeline komposisi standar.
- **Solusi**: Kita melakukan *Rollback* cepat ke model `WS_CHILD` yang stabil. Kadang, batasan platform adalah panduan, bukan rintangan.

### 🦉 Kebijaksanaan: "Dua Wajah, Satu Jiwa" (Ikonografi)
Kita belajar bahwa ikon aplikasi bukan hanya satu file gambar.
- **App Icon**: Untuk identitas luar (Taskbar/Explorer).
- **In-App Icon**: Untuk pengalaman dalam (Title Bar).
- **Pelajaran**: Memisahkan `hIcon` dan `hIconSm` di level `RegisterClassExW` adalah kunci untuk menjaga ketajaman visual di berbagai ukuran tanpa kompromi kualitas.

### 💎 Praktik Terbaik Baru
- **Spring Over Linear**: Selalu gunakan pegas untuk elemen yang bergerak mengikuti interaksi user (seperti tab atau border gap). Ini membuat aplikasi terasa "hidup".
- **Surgical Rollback**: Jangan ragu untuk menarik kembali fitur eksperimental yang merusak stabilitas. Stabilitas adalah fitur nomor satu.
