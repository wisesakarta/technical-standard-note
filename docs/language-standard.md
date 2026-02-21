# Documentation Language Standard
# Standar Bahasa Dokumentasi

## Purpose / Tujuan

This file defines language rules for public notes and documentation in Saka Note.  
File ini menetapkan aturan bahasa untuk catatan dan dokumentasi publik di Saka Note.

Goal / Tujuan:
- Keep docs clear for global readers (English) and local collaborators (Indonesia).
- Menjaga dokumen tetap jelas untuk pembaca global (English) dan kolaborator lokal (Indonesia).

## Scope / Cakupan

Apply to / Berlaku untuk:
- `README.md`
- `docs/*.md`
- `research/*.md`
- Release notes and benchmark reports / Catatan rilis dan laporan benchmark

## Core Rules / Aturan Inti

1. English first, Indonesian companion.
   - Tulis poin utama dalam English, lalu sediakan padanan Indonesia tepat di bawahnya.
2. Keep both versions semantically equivalent.
   - Makna English dan Indonesia harus setara, tidak boleh saling bertentangan.
3. Avoid slang, profanity, and personal attacks.
   - Hindari slang, kata kasar, dan serangan personal.
4. Keep terminology consistent.
   - Gunakan istilah yang konsisten di semua dokumen.
5. Prefer short, direct sentences.
   - Utamakan kalimat pendek dan langsung.

## Recommended Structure / Struktur yang Disarankan

For new sections:
- `## Section Title / Judul Bagian`
- One concise English paragraph.
- Satu paragraf Indonesia yang ringkas.

For bullet lists:
- Either paired bilingual bullets, or
- Gunakan bullet berpasangan bilingual, atau
- English bullets followed by an Indonesian summary block.
- bullet English diikuti blok ringkasan Indonesia.

## Canonical Terms / Istilah Baku

- Lightweight = Ringan
- Startup behavior = Perilaku startup
- Session restore = Pemulihan sesi
- Benchmark gate = Gerbang benchmark
- Release artifact = Artefak rilis
- Tab workflow = Alur kerja tab
- File I/O = I/O file

## Review Checklist / Checklist Review

Before merge:
- English and Indonesian are both present where needed.
- English dan Indonesia hadir pada bagian yang diperlukan.
- Technical terms are consistent with this file.
- Istilah teknis konsisten dengan file ini.
- No informal insults or ambiguous wording.
- Tidak ada hinaan informal atau kalimat ambigu.
