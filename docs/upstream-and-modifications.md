# Upstream Attribution and Modifications
# Atribusi Upstream dan Modifikasi

## Upstream Project / Proyek Upstream

- Original repository / Repositori asli: https://github.com/ForLoopCodes/legacy-notepad
- Original author/maintainer / Penulis & pemelihara asli: ForLoopCodes
- License / Lisensi: MIT (this project remains MIT-compliant / proyek ini tetap patuh MIT)

## This Project / Proyek Ini

- Project name / Nama proyek: Saka Note
- Maintainer / Pemelihara: Saka Studio & Engineering
- Product direction / Arah produk: lightweight, plain-text-first Windows editor / editor Windows ringan dengan fokus teks polos

## Major Modification Areas / Area Modifikasi Utama

The codebase has been significantly extended and reorganized from the upstream baseline.  
Codebase telah diperluas dan direorganisasi secara signifikan dari baseline upstream.

- Session behavior and autosave reliability improvements.
- Peningkatan perilaku sesi dan reliabilitas autosave.
- Tab workflow and tab interaction refinements.
- Penyempurnaan alur kerja tab dan interaksi tab.
- File I/O hardening (partial read/write handling).
- Penguatan I/O file (penanganan baca/tulis parsial).
- Startup behavior options (classic/resume modes).
- Opsi perilaku startup (mode classic/resume).
- Internal benchmark gate (`--benchmark-ci`) and A/B benchmark tooling.
- Benchmark gate internal (`--benchmark-ci`) dan tooling benchmark A/B.
- UI consistency improvements (dialogs, tab visuals, dark mode polish).
- Peningkatan konsistensi UI (dialog, visual tab, dark mode polish).
- Build/dev workflow additions (benchmark utilities, cleanup scripts, CI release artifacts).
- Penambahan workflow build/dev (utilitas benchmark, script cleanup, artifact rilis CI).
- Branding updates from upstream identity to Saka Note.
- Pembaruan branding dari identitas upstream ke Saka Note.

## Compatibility Notes / Catatan Kompatibilitas

- Some internal names and structures still preserve legacy references for migration stability.
- Beberapa nama dan struktur internal masih menyimpan referensi legacy untuk stabilitas migrasi.
- Registry/config/session locations are migrated toward `SakaNote` naming while keeping fallback compatibility from legacy keys.
- Lokasi registry/config/session dimigrasikan ke nama `SakaNote` sambil menjaga kompatibilitas fallback dari key legacy.

## Documentation Policy / Kebijakan Dokumentasi

When publishing releases, include / Saat merilis versi baru, sertakan:

- Upstream attribution link above.
- Link atribusi upstream di atas.
- Summary of changes in release notes.
- Ringkasan perubahan pada release notes.
- Benchmark evidence for performance claims (`comparison.md` + raw CSV outputs).
- Bukti benchmark untuk klaim performa (`comparison.md` + output CSV mentah).
