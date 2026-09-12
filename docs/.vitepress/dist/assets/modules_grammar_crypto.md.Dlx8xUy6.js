import{_ as s,o as e,c as n,a0 as t}from"./chunks/framework.BpJS36ta.js";const h=JSON.parse('{"title":"Crypto Module Grammar","description":"","frontmatter":{},"headers":[],"relativePath":"modules/grammar/crypto.md","filePath":"modules/grammar/crypto.md"}'),r={name:"modules/grammar/crypto.md"};function p(l,a,o,c,i,d){return e(),n("div",null,[...a[0]||(a[0]=[t(`<h1 id="crypto-module-grammar" tabindex="-1">Crypto Module Grammar <a class="header-anchor" href="#crypto-module-grammar" aria-label="Permalink to &quot;Crypto Module Grammar&quot;">​</a></h1><h2 id="ast-structure" tabindex="-1">AST Structure <a class="header-anchor" href="#ast-structure" aria-label="Permalink to &quot;AST Structure&quot;">​</a></h2><h3 id="crypto-hash-str" tabindex="-1"><code>crypto.hash(str)</code> <a class="header-anchor" href="#crypto-hash-str" aria-label="Permalink to &quot;\`crypto.hash(str)\`&quot;">​</a></h3><div class="language-text vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>Call:</span></span>
<span class="line"><span>  Callee:</span></span>
<span class="line"><span>    Member:</span></span>
<span class="line"><span>      Object:</span></span>
<span class="line"><span>        Identifier: crypto</span></span>
<span class="line"><span>      Member:</span></span>
<span class="line"><span>        Identifier: hash</span></span>
<span class="line"><span>  Arg 1:</span></span>
<span class="line"><span>    String: &quot;hello&quot;</span></span></code></pre></div><h2 id="module-structure" tabindex="-1">Module Structure <a class="header-anchor" href="#module-structure" aria-label="Permalink to &quot;Module Structure&quot;">​</a></h2><div class="language-text vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>crypto</span></span>
<span class="line"><span>├── hash(str)           — DJB2 hash</span></span>
<span class="line"><span>├── fnv1a(str)          — FNV-1a hash</span></span>
<span class="line"><span>├── murmur3(str, seed?) — MurmurHash3</span></span>
<span class="line"><span>├── xor(str, key)       — XOR cipher</span></span>
<span class="line"><span>├── base64Encode(str)   — Base64 encode</span></span>
<span class="line"><span>└── base64Decode(str)   — Base64 decode</span></span></code></pre></div>`,6)])])}const m=s(r,[["render",p]]);export{h as __pageData,m as default};
