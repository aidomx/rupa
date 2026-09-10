import{_ as i,o as t,c as l,a0 as s,j as n,a as e,t as r}from"./chunks/framework.CShDTZbC.js";const k=JSON.parse('{"title":"Print Grammar","description":"","frontmatter":{},"headers":[],"relativePath":"grammar/print.md","filePath":"grammar/print.md"}'),o={name:"grammar/print.md"},c={id:"expression-interpolation-expr",tabindex:"-1"};function d(p,a,u,g,h,m){return t(),l("div",null,[a[3]||(a[3]=s(`<h1 id="print-grammar" tabindex="-1">Print Grammar <a class="header-anchor" href="#print-grammar" aria-label="Permalink to &quot;Print Grammar&quot;">​</a></h1><p>Grammar <code>print</code> membentuk node <code>Print</code> dari satu atau lebih expression yang dipisahkan oleh koma.</p><p>Setiap expression menjadi child langsung dari node <code>Print</code>. Koma tidak menghasilkan node AST tersendiri.</p><h2 id="single-argument" tabindex="-1">Single argument <a class="header-anchor" href="#single-argument" aria-label="Permalink to &quot;Single argument&quot;">​</a></h2><p>Source:</p><div class="language-rupa vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">rupa</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>print(&#39;hello world\\n&#39;);</span></span></code></pre></div><p>AST:</p><div class="language-text vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>Program:</span></span>
<span class="line"><span>  Print:</span></span>
<span class="line"><span>    String: &quot;hello world\\n&quot;</span></span></code></pre></div><p>Satu argument menghasilkan satu child pada node <code>Print</code>.</p><h2 id="multiple-arguments" tabindex="-1">Multiple arguments <a class="header-anchor" href="#multiple-arguments" aria-label="Permalink to &quot;Multiple arguments&quot;">​</a></h2><p>Source:</p><div class="language-rupa vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">rupa</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>print(&#39;x =&#39;, x, &#39;\\n&#39;);</span></span></code></pre></div><p>AST:</p><div class="language-text vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>Program:</span></span>
<span class="line"><span>  Print:</span></span>
<span class="line"><span>    String: &quot;x =&quot;</span></span>
<span class="line"><span>    Literal ID: x</span></span>
<span class="line"><span>    String: &quot;\\n&quot;</span></span></code></pre></div><p>Setiap argument dipertahankan sebagai node terpisah di dalam <code>Print</code>.</p><p>Dengan demikian, bentuk:</p><div class="language-rupa vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">rupa</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>print(a, b, c);</span></span></code></pre></div><p>direpresentasikan sebagai:</p><div class="language-text vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>Print:</span></span>
<span class="line"><span>  &lt;expression a&gt;</span></span>
<span class="line"><span>  &lt;expression b&gt;</span></span>
<span class="line"><span>  &lt;expression c&gt;</span></span></code></pre></div><h2 id="argument-berupa-expression" tabindex="-1">Argument berupa expression <a class="header-anchor" href="#argument-berupa-expression" aria-label="Permalink to &quot;Argument berupa expression&quot;">​</a></h2><p>Argument <code>print</code> tidak terbatas pada literal. Sebuah argument dapat berupa expression yang menghasilkan node AST sendiri.</p><p>Source:</p><div class="language-rupa vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">rupa</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>print(name + &#39;\\n&#39;);</span></span></code></pre></div><p>AST:</p><div class="language-text vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>Program:</span></span>
<span class="line"><span>  Print:</span></span>
<span class="line"><span>    Binary: +</span></span>
<span class="line"><span>      Left:</span></span>
<span class="line"><span>        Literal ID: name</span></span>
<span class="line"><span>      Right:</span></span>
<span class="line"><span>        String: &quot;\\n&quot;</span></span></code></pre></div><p>Expression tersebut tetap menjadi satu argument <code>Print</code>, sedangkan struktur operasinya direpresentasikan oleh node <code>Binary</code>.</p><h2 id="multiple-arguments-dengan-expression" tabindex="-1">Multiple arguments dengan expression <a class="header-anchor" href="#multiple-arguments-dengan-expression" aria-label="Permalink to &quot;Multiple arguments dengan expression&quot;">​</a></h2><p>Source:</p><div class="language-rupa vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">rupa</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>print(&#39;result:&#39;, x + y, true);</span></span></code></pre></div><p>AST:</p><div class="language-text vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>Program:</span></span>
<span class="line"><span>  Print:</span></span>
<span class="line"><span>    String: &quot;result:&quot;</span></span>
<span class="line"><span>    Binary: +</span></span>
<span class="line"><span>      Left:</span></span>
<span class="line"><span>        Literal ID: x</span></span>
<span class="line"><span>      Right:</span></span>
<span class="line"><span>        Literal ID: y</span></span>
<span class="line"><span>    Boolean: true</span></span></code></pre></div><p>Koma memisahkan tiga argument:</p><ol><li><code>&quot;result:&quot;</code></li><li><code>x + y</code></li><li><code>true</code></li></ol><p>Operator <code>+</code> hanya berlaku di dalam argument kedua dan menghasilkan <code>Binary</code> node.</p><h2 id="function-call-sebagai-argument" tabindex="-1">Function call sebagai argument <a class="header-anchor" href="#function-call-sebagai-argument" aria-label="Permalink to &quot;Function call sebagai argument&quot;">​</a></h2><p>Source:</p><div class="language-rupa vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">rupa</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>print(add(1, 2));</span></span></code></pre></div><p>AST:</p><div class="language-text vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>Program:</span></span>
<span class="line"><span>  Print:</span></span>
<span class="line"><span>    Call:</span></span>
<span class="line"><span>      Callee:</span></span>
<span class="line"><span>        Identifier: add</span></span>
<span class="line"><span>      Arg 1:</span></span>
<span class="line"><span>        Number: 1</span></span>
<span class="line"><span>      Arg 2:</span></span>
<span class="line"><span>        Number: 2</span></span></code></pre></div><p>Function call merupakan satu expression dan menjadi satu argument pada <code>Print</code>.</p><h2 id="expression-kompleks" tabindex="-1">Expression kompleks <a class="header-anchor" href="#expression-kompleks" aria-label="Permalink to &quot;Expression kompleks&quot;">​</a></h2><p>Argument dapat berupa expression yang terdiri dari beberapa operasi.</p><p>Source:</p><div class="language-rupa vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">rupa</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>print(&#39;result:&#39; + x + y + &#39; true\\n&#39;);</span></span></code></pre></div><p>AST:</p><div class="language-text vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>Program:</span></span>
<span class="line"><span>  Print:</span></span>
<span class="line"><span>    Binary: +</span></span>
<span class="line"><span>      Left:</span></span>
<span class="line"><span>        Binary: +</span></span>
<span class="line"><span>          Left:</span></span>
<span class="line"><span>            Binary: +</span></span>
<span class="line"><span>              Left:</span></span>
<span class="line"><span>                String: &quot;result:&quot;</span></span>
<span class="line"><span>              Right:</span></span>
<span class="line"><span>                Literal ID: x</span></span>
<span class="line"><span>          Right:</span></span>
<span class="line"><span>            Literal ID: y</span></span>
<span class="line"><span>      Right:</span></span>
<span class="line"><span>        String: &quot; true\\n&quot;</span></span></code></pre></div><p>Struktur <code>Binary</code> mengikuti pengelompokan expression yang dibentuk parser.</p><h2 id="function-call-dalam-expression" tabindex="-1">Function call dalam expression <a class="header-anchor" href="#function-call-dalam-expression" aria-label="Permalink to &quot;Function call dalam expression&quot;">​</a></h2><p>Function call juga dapat digunakan sebagai bagian dari expression argument.</p><p>Source:</p><div class="language-rupa vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">rupa</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>print(add(1, 2) + &#39; &#39; + [3, 4] + &#39;\\n&#39;);</span></span></code></pre></div><p>AST:</p><div class="language-text vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>Program:</span></span>
<span class="line"><span>  Print:</span></span>
<span class="line"><span>    Binary: +</span></span>
<span class="line"><span>      Left:</span></span>
<span class="line"><span>        Binary: +</span></span>
<span class="line"><span>          Left:</span></span>
<span class="line"><span>            Binary: +</span></span>
<span class="line"><span>              Left:</span></span>
<span class="line"><span>                Call:</span></span>
<span class="line"><span>                  Callee:</span></span>
<span class="line"><span>                    Identifier: add</span></span>
<span class="line"><span>                  Arg 1:</span></span>
<span class="line"><span>                    Number: 1</span></span>
<span class="line"><span>                  Arg 2:</span></span>
<span class="line"><span>                    Number: 2</span></span>
<span class="line"><span>              Right:</span></span>
<span class="line"><span>                String: &quot; &quot;</span></span>
<span class="line"><span>          Right:</span></span>
<span class="line"><span>            Number: 3</span></span>
<span class="line"><span>      Right:</span></span>
<span class="line"><span>        String: &quot;\\n&quot;</span></span></code></pre></div><p><code>Print</code> hanya mengetahui bahwa seluruh expression tersebut merupakan satu argument. Struktur operasi di dalamnya ditentukan oleh node expression seperti <code>Binary</code> dan <code>Call</code>.</p><h2 id="string-interpolation" tabindex="-1">String interpolation <a class="header-anchor" href="#string-interpolation" aria-label="Permalink to &quot;String interpolation&quot;">​</a></h2><p>String literal dapat berisi interpolation untuk menampilkan nilai variabel atau hasil ekspresi.</p><h3 id="variable-interpolation-name" tabindex="-1">Variable interpolation: <code>{name}</code> <a class="header-anchor" href="#variable-interpolation-name" aria-label="Permalink to &quot;Variable interpolation: \`{name}\`&quot;">​</a></h3><p>Source:</p><div class="language-rupa vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">rupa</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>print(&#39;Hello {name}\\n&#39;);</span></span></code></pre></div><p>AST:</p><div class="language-text vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>Program:</span></span>
<span class="line"><span>  Print:</span></span>
<span class="line"><span>    StringInterp:</span></span>
<span class="line"><span>      String: &quot;Hello &quot;</span></span>
<span class="line"><span>      Literal ID: name</span></span>
<span class="line"><span>      String: &quot;\\n&quot;</span></span></code></pre></div>`,61)),n("h3",c,[a[0]||(a[0]=e("Expression interpolation: ",-1)),n("code",null,r(p.expr),1),a[1]||(a[1]=e()),a[2]||(a[2]=n("a",{class:"header-anchor",href:"#expression-interpolation-expr","aria-label":'Permalink to "Expression interpolation: `{{expr}}`"'},"​",-1))]),a[4]||(a[4]=s(`<p>Source:</p><div class="language-rupa vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">rupa</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>print(&#39;{{add(1,2)}}\\n&#39;);</span></span>
<span class="line"><span>print(&#39;{{user.name}}&#39;);</span></span>
<span class="line"><span>print(&#39;{{x + y}}&#39;);</span></span></code></pre></div><p>AST:</p><div class="language-text vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>Program:</span></span>
<span class="line"><span>  Print:</span></span>
<span class="line"><span>    StringInterp:</span></span>
<span class="line"><span>      Call:</span></span>
<span class="line"><span>        Callee: Identifier: add</span></span>
<span class="line"><span>        Arg 1: Number: 1</span></span>
<span class="line"><span>        Arg 2: Number: 2</span></span>
<span class="line"><span>      String: &quot;\\n&quot;</span></span>
<span class="line"><span>  Print:</span></span>
<span class="line"><span>    StringInterp:</span></span>
<span class="line"><span>      Member:</span></span>
<span class="line"><span>        Object:</span></span>
<span class="line"><span>          Identifier: user</span></span>
<span class="line"><span>        Member:</span></span>
<span class="line"><span>          Identifier: name</span></span>
<span class="line"><span>  Print:</span></span>
<span class="line"><span>    StringInterp:</span></span>
<span class="line"><span>      Binary: +</span></span>
<span class="line"><span>        Left:</span></span>
<span class="line"><span>          Literal ID: x</span></span>
<span class="line"><span>        Right:</span></span>
<span class="line"><span>          Literal ID: y</span></span></code></pre></div><h3 id="mixed-literal-dan-interpolation" tabindex="-1">Mixed literal dan interpolation <a class="header-anchor" href="#mixed-literal-dan-interpolation" aria-label="Permalink to &quot;Mixed literal dan interpolation&quot;">​</a></h3><p>Source:</p><div class="language-rupa vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">rupa</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>print(&#39;Hello {name}, you have {{count}} items\\n&#39;);</span></span></code></pre></div><p>AST:</p><div class="language-text vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>Program:</span></span>
<span class="line"><span>  Print:</span></span>
<span class="line"><span>    StringInterp:</span></span>
<span class="line"><span>      String: &quot;Hello &quot;</span></span>
<span class="line"><span>      Literal ID: name</span></span>
<span class="line"><span>      String: &quot;, you have &quot;</span></span>
<span class="line"><span>      Literal ID: count</span></span>
<span class="line"><span>      String: &quot; items\\n&quot;</span></span></code></pre></div><p>Node <code>StringInterp</code> berisi array parts yang ber alternating antara <code>String</code> (literal text) dan expression nodes (variable, call, binary, member, dll).</p><h2 id="struktur-umum" tabindex="-1">Struktur umum <a class="header-anchor" href="#struktur-umum" aria-label="Permalink to &quot;Struktur umum&quot;">​</a></h2><p>Secara konseptual, grammar <code>print</code> dapat dipandang sebagai:</p><div class="language-text vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>print</span></span>
<span class="line"><span>  └── expression (, expression)*</span></span></code></pre></div><p>dan struktur AST-nya:</p><div class="language-text vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>Print</span></span>
<span class="line"><span>  ├── Expression</span></span>
<span class="line"><span>  ├── Expression</span></span>
<span class="line"><span>  └── Expression</span></span></code></pre></div><p>Jumlah child <code>Print</code> mengikuti jumlah argument yang diberikan.</p><h3 id="contoh" tabindex="-1">Contoh <a class="header-anchor" href="#contoh" aria-label="Permalink to &quot;Contoh&quot;">​</a></h3><p>Source:</p><div class="language-rupa vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">rupa</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>print(a, b + c, add(1, 2));</span></span></code></pre></div><p>Struktur AST:</p><div class="language-text vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>Program:</span></span>
<span class="line"><span>  Print:</span></span>
<span class="line"><span>    Literal ID: a</span></span>
<span class="line"><span>    Binary: +</span></span>
<span class="line"><span>      Left:</span></span>
<span class="line"><span>        Literal ID: b</span></span>
<span class="line"><span>      Right:</span></span>
<span class="line"><span>        Literal ID: c</span></span>
<span class="line"><span>    Call:</span></span>
<span class="line"><span>      Callee:</span></span>
<span class="line"><span>        Identifier: add</span></span>
<span class="line"><span>      Arg 1:</span></span>
<span class="line"><span>        Number: 1</span></span>
<span class="line"><span>      Arg 2:</span></span>
<span class="line"><span>        Number: 2</span></span></code></pre></div><p>Dengan struktur tersebut, <code>print</code> berperan sebagai node statement yang menampung daftar expression, sementara grammar expression tetap bertanggung jawab membentuk struktur masing-masing argument.</p>`,22))])}const v=i(o,[["render",d]]);export{k as __pageData,v as default};
