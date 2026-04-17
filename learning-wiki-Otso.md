# Learning Wiki: Otso 🍯

Selamat datang di jurnal pembelajaran proyek **Otso**. Dokumen ini adalah "blok memori" kolektif kita—tempat kita menyimpan alasan di balik keputusan teknis, cerita dari medan tempur debugging, dan kebijaksanaan yang kita petik sepanjang jalan.

## 🏙️ Arsitektur (Peta Kota)

Otso dirancang seperti **"Jam Tangan Mekanik"**:
- **Win32 Native**: Tidak ada baterai (framework berat), tidak ada layar digital (elektron). Semuanya adalah roda gigi C++ yang presisi.
- **UI Modular**: Setiap komponen (editor, tab, scrollbar) adalah modul mandiri yang diorkestrasikan oleh `main.cpp`.
- **Renaissance Philosophy**: Menghargai detail kecil. Scrollbar yang halus, aura seleksi yang dinamis, tapi tetap ringan seperti udara.

## 🛠️ Keputusan Teknis (Alasan Mengapa)

### **Warna Aksen: Otso Amber (#EEBA2C)**
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

Sesi malam ini adalah tentang **"Stabilitas & Identitas"**. Kita mencoba mendorong batas visual Otso ke ekstremitas baru, belajar dari kegagalan, dan menyiapkan panggung untuk identitas brand yang lebih kuat.

### 🏙️ Arsitektur: PremiumOrchestrator & Spring Physics
Kita telah memperkenalkan `PremiumOrchestrator` sebagai konduktor utama untuk efek visual tinggi. Selain itu, **Spring Physics** (Second-Order Dynamics) sekarang menjadi bagian dari DNA animasi Otso, memungkinkan gerakan UI yang memiliki massa dan momentum, bukan hanya durasi statis.

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

---

## Log Pembaruan: 15 April 2026, 23:10 (The Blueprint Transition)

Malam ini kita bergeser dari kehangatan Amber ke kedinginan presisi **Blueprint Blue (#001ae2)**. Ini bukan sekadar perubahan warna, tapi pergeseran tone produk.

### 🏙️ Arsitektur: Identitas "Technical Ink"
Warna aksen baru ini memberikan nuansa cetak biru teknis. Kita menggunakan format BGR `0xE21A00` untuk memastikan konsistensi pada level rendering Win32.

### 🦉 Kebijaksanaan: Lexicon Branding Insights
Kita mengeksplorasi teknik penamaan brand premium:
- **Phonetic Simplicity**: Nama harus mudah diucapkan tapi memiliki "vibrasi" yang kuat (seperti pola "S" dan "L" pada Otso atau konsonan tajam pada calon nama baru).
- **Polarization**: Sebuah brand premium tidak harus disukai semua orang, tapi harus terasa "benar" bagi target penggunanya (insinyur, penulis teknis).

### 💎 Praktik Terbaik Baru
- **Surgical Code Implementation**: Selalu gunakan prinsip Karpathy—ubah sesedikit mungkin baris kode (orthogonality) untuk mencapai dampak maksimal.
---

## Log Pembaruan: 16 April 2026 (The Floating Precision)

Sesi ini kita berfokus pada transisi dari UI yang fungsional menjadi UI yang memiliki karakter **"Premium Floating"**. Kita belajar bahwa ruang kosong (negative space) sama pentingnya dengan elemen visual itu sendiri.

### 🏙️ Arsitektur: Floating Layout & Semantic Spacing
Kita mengimplementasikan sistem di mana Editor dan Scrollbar tidak lagi menempel pada tepi jendela. Menggunakan `DesignSystem::kGlobalMarginPx` (12px), kita memberikan "bingkai" pada area kerja. Ini membuat aplikasi terasa lebih seperti alat desain daripada sekadar notepad.

### 🐛 Cerita Perang: "The Botched Brace"
Saat mencoba mengotomatisasi refaktoring pada `ui.cpp`, orkestrasi tools sempat menduplikasi blok logika status bar dan meninggalkan kurung kurawal yang tidak tertutup.
- **Gejala**: Compiler error yang membingungkan seolah-olah seluruh file rusak.
- **Penyebab**: Target penggantian string yang tumpang tindih dalam satu siklus edit.
- **Solusi**: Surgical manual fix. Pelajaran: "Jangan terlalu percaya pada otomasi saat mengedit blok kontrol aliran (if/else) yang besar."

### 🦉 Kebijaksanaan: "Breathing Room" dalam Rekayasa
Kita belajar bahwa dalam aplikasi teknis, kepadatan informasi (density) harus diseimbangkan dengan kejelasan. Jarak 12px adalah "Golden Ratio" kita hari ini—cukup rapat untuk efisiensi, cukup luas untuk kelegaan visual.

### 💎 Praktik Terbaik Baru
- **Zero Hardcoded Pixels**: Mulai sekarang, setiap angka pixel di logika UI **WAJIB** berasal dari `DesignSystem`. Jika angka itu tidak ada, tambahkan sebagai token baru.
- **Animation Calibration**: Gerakan 250ms terkadang terasa lambat untuk brand teknis. Gunakan standar **150ms** untuk transisi "reveal" agar terasa lebih instan dan presisi.

---

## Log Pembaruan: 16 April 2026, 16:30 (Renaissance Physics)

Sesi ini menandai transisi Otso dari animasi tradisional ke **Second-Order Dynamics**. Kita membangun mesin fisika kecil yang membuat UI terasa memiliki "jiwa" dan massa.

### 🏙️ Arsitektur: Core Spring Module
Kita telah memindahkan logika fisika ke modul murni `src/core/spring_solver.h`. Menggunakan integrasi *Semi-Implicit Euler*, kita mencapai stabilitas numerik yang memungkinkan UI meluncur tanpa cacat.

### 🛠️ Keputusan Teknis: Why Springs?
- **Interruptible**: Pengguna tidak perlu menunggu animasi selesai (blocking). Jika target berubah saat animasi berjalan (seperti mengeklik tab lain), pegas akan mengalihkan momentumnya secara natural ke target baru.
- **Organic Feel**: Tidak ada lagi percepatan/perlambatan linear yang kaku. Semuanya mengikuti hukum Hooke: $F = -kx$.

### 🐛 Cerita Perang: "The Ghost Function" & "Locked Disk"
- **Gejala**: Kode gagal dikompilasi dengan error `'GetTabContentRect' was not declared in this scope`.
- **Penyebab**: Kita memanggil fungsi tersebut di bagian atas file (`main.cpp`) sementara definisinya berada jauh di bawah. C++ membutuhkan deklarasi maju (*forward declaration*) untuk hal ini.
- **Insiden Permission Denied**: Kompiler gagal menimpa `Otso.exe` karena aplikasi masih berjalan di background saat kita mencoba rebuild.
- **Pelajaran**: "Clean Build" seringkali berarti "Clean Environment". Selalu pastikan semua instance proses lama sudah mati sebelum melakukan integrasi biner baru.

### 🦉 Kebijaksanaan: "Momentum is UX"
Kita belajar bahwa desain bukan hanya tentang bagaimana sesuatu terlihat saat diam, tapi bagaimana ia merespons saat digerakkan. Memberikan *overshoot* kecil pada tab atau logo memberikan umpan balik taktil yang membuat user merasa memegang kendali penuh.

### 💎 Praktik Terbaik Baru
- **Initialize Springs to Reality**: Saat merekonstruksi UI (seperti startup), inisialisasi pegas langsung ke posisi target (`Reset()`) agar tidak terjadi gerakan meluncur liar yang tidak diinginkan di awal.
- **Physics Update frequency**: Gunakan delta-time tetap (misal 16ms) untuk fisika guna menjamin konsistensi gerakan di berbagai kecepatan monitor.

---

## Log Pembaruan: 16 April 2026, 21:00 (Modular Renaissance)

Sesi ini adalah tentang **"Restorasi & Modularisasi"**. Kita berhasil mengembalikan estetika skeuomorphic yang hilang dan membungkusnya dalam arsitektur yang jauh lebih bersih.

### 🏙️ Arsitektur: Component-Based Architecture (Saka-Modular)
Kita memecah `EditorApp.kt` yang monolitik menjadi paket `com.wisesakarta.Otso.ui.components`. Sekarang setiap elemen (Folder, Panels, Feedback, Dialogs) memiliki rumahnya sendiri. Ini bukan hanya tentang rapi, tapi tentang **Cognitive Load Reduction**.

### 🛠️ Keputusan Teknis: Room Restoration & KAPT
Kita memutuskan untuk mengembalikan Room Database sebagai jantung data Otso Android.
- **Nested Hierarchies**: Menggunakan `folder_id` yang rekursif untuk mendukung folder di dalam folder.
- **KAPT vs Baseline**: Seringkali status "Baseline" proyek menghapus dependensi krusial. Mengembalikan Room memerlukan penambahan plugin `kapt` dan dependensi runtime secara manual di `build.gradle.kts`.

### 🐛 Cerita Perang: "The Dependency Amnesia"
Setelah refaktoring besar selesai, aplikasi gagal dibangun dengan ribuan error "Unresolved reference: Room".
- **Penyebab**: Kita bermigrasi ke workspace yang belum pernah mengenal `Room`, sehingga compiler tidak tahu apa itu `@Dao` atau `RoomDatabase`.
- **Solusi**: "Surgical Injection" pada `build.gradle.kts`. Menambahkan plugin `org.jetbrains.kotlin.kapt` dan dependensi Room versi 2.6.1.
- **Pelajaran**: "Refaktor kode tidak cukup jika build-script tertinggal." Selalu audit `dependencies` saat bermigrasi fitur antar proyek.

### 🦉 Kebijaksanaan: Restorasi adalah Arkeologi Digital
Mengembalikan fitur dari backup bukan sekadar copy-paste. Kita harus memahami konteks di mana fitur itu hidup. Dalam kasus Otso, folder skeuomorphic membutuhkan "tulang punggung" data yang kuat. Tanpa Room, UI hanyalah cangkang kosong yang lambat laun akan pecah.

### 💎 Praktik Terbaik Baru
- **Import with Precision**: Hindari import wildcard (`.*`) jika memungkinkan, kecuali untuk paket animasi Compose yang sangat padat. Ini membantu tracking dependensi saat melakukan debugging build.
- **Explicit Lambda Parameters**: Kotlin compiler kadang bingung melakukan inferensi tipe pada lambda yang bersarang. Memberikan nama parameter eksplisit (seperti `{ f: FolderEntity -> ... }`) adalah tanda kematangan kode dan membantu compiler bekerja lebih cepat.
