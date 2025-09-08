# Sejarah Rupa Language

## Pembahasan

**Rupa** awalnya dibangun dengan **NodeJS** dan hanya digunakan sebagai pendukung framework **NextJS**,  
khususnya untuk membuat komponen berbasis _rules_.

Seiring waktu, [aidomx](https://github.com/aidomx/aidomx) yang semula ditargetkan untuk membantu project **NextJS**,  
mengalami kendala dalam menangani fitur reaktif seperti efek klik dan interaksi lain.

Dari percobaan tersebut lahirlah konsep **Ghost Component**.  
Konsep ini terbukti berhasil: mampu membuat hingga **1 juta komponen**  
dan bahkan memberikan performa lebih cepat dibanding sistem bawaan NextJS.

Namun, ada satu masalah besar: sisi reaktifnya **belum berhasil** dihidupkan.  
Komponen yang tidak bisa reaktif pada akhirnya menjadi terbatas kegunaannya.

Setelah berdiskusi panjang dengan **ChatGPT**, saya diarahkan untuk mencoba membuat **DSL** sendiri.  
Awalnya, DSL tersebut ditulis dengan bahasa C dan digunakan sebagai compiler untuk project NodeJS.  
Meski berhasil, performanya masih belum sesuai harapan, dan masalah reaktif juga belum sepenuhnya terpecahkan.

Dari titik itu saya menyadari bahwa ekosistem yang saya bangun sudah berbeda.  
Daripada terus memaksa di atas fondasi lama, saya memutuskan untuk membangun **bahasa baru dari nol**.

Konsep **Ghost Component** tetap menjadi inspirasi penting.  
Dengan konfigurasi sederhana, sistem dapat langsung menyediakan komponen siap pakai tanpa harus membuat banyak berkas.  
Kini, dengan bahasa sendiri, konsep tersebut bisa terus dikembangkan dan diintegrasikan secara penuh.

---

## Timeline Singkat

- **Fase 1** → Rupa dibangun di atas NodeJS untuk mendukung NextJS (komponen berbasis rules).
- **Fase 2** → Lahir konsep **Ghost Component**, mampu membuat hingga 1 juta komponen.
- **Fase 3** → Kendala besar: sisi **reaktif belum berhasil** berjalan.
- **Fase 4** → Diskusi dengan ChatGPT → diarahkan membuat **DSL**.
- **Fase 5** → DSL pertama dibuat dengan bahasa C, dipakai sebagai compiler untuk NodeJS,  
  tapi performa dan reaktif masih terbatas.
- **Fase 6** → Disadari ekosistem sudah berbeda → keputusan besar: **membangun bahasa sendiri**.

---

_Selamat datang di Rupa._
