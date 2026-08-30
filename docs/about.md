# About Rupa

Rupa berawal dari eksperimen membangun sistem komponen untuk proyek berbasis NextJS. Pada fase awal, pendekatan tersebut dibangun di atas NodeJS dan menggunakan aturan untuk membentuk komponen.

Dari eksperimen itu lahir konsep **Ghost Component**: ide untuk menyediakan banyak komponen dari konfigurasi tanpa harus membuat banyak berkas secara manual. Eksperimen tersebut mendorong pengembangan ke arah sistem yang lebih reaktif, tetapi keterbatasan fondasi awal membuat arah proyek terus berubah.

Tahap berikutnya adalah percobaan membuat DSL dengan C sebagai bagian dari toolchain proyek sebelumnya. Setelah fondasi dan kebutuhan sistem semakin berbeda dari lingkungan asalnya, keputusan akhirnya adalah membangun bahasa sendiri dari nol: Rupa.

## Perjalanan singkat

1. Rupa berawal dari eksperimen pendukung komponen berbasis NodeJS/NextJS.
2. Konsep Ghost Component menjadi salah satu inspirasi desain penting.
3. Kebutuhan reaktivitas dan performa mendorong eksperimen DSL.
4. DSL awal dibangun menggunakan C.
5. Fondasi proyek berkembang melampaui kebutuhan sistem sebelumnya.
6. Rupa kemudian dibangun sebagai bahasa pemrograman mandiri.

Dokumen ini menjelaskan latar belakang, bukan spesifikasi teknis. Untuk implementasi dan kontribusi, lihat [instruction.md](instruction.md) dan [syntax/](syntax/index.md).
