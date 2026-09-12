import{_ as s,o as n,c as e,a0 as t}from"./chunks/framework.BpJS36ta.js";const h=JSON.parse('{"title":"Regex Module Grammar","description":"","frontmatter":{},"headers":[],"relativePath":"modules/grammar/regex.md","filePath":"modules/grammar/regex.md"}'),p={name:"modules/grammar/regex.md"};function r(l,a,i,c,d,o){return n(),e("div",null,[...a[0]||(a[0]=[t(`<h1 id="regex-module-grammar" tabindex="-1">Regex Module Grammar <a class="header-anchor" href="#regex-module-grammar" aria-label="Permalink to &quot;Regex Module Grammar&quot;">​</a></h1><h2 id="ast-structure" tabindex="-1">AST Structure <a class="header-anchor" href="#ast-structure" aria-label="Permalink to &quot;AST Structure&quot;">​</a></h2><h3 id="regex-match-pattern-str" tabindex="-1"><code>regex.match(pattern, str)</code> <a class="header-anchor" href="#regex-match-pattern-str" aria-label="Permalink to &quot;\`regex.match(pattern, str)\`&quot;">​</a></h3><div class="language-text vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>Call:</span></span>
<span class="line"><span>  Callee:</span></span>
<span class="line"><span>    Member:</span></span>
<span class="line"><span>      Object:</span></span>
<span class="line"><span>        Identifier: regex</span></span>
<span class="line"><span>      Member:</span></span>
<span class="line"><span>        Identifier: match</span></span>
<span class="line"><span>  Arg 1:</span></span>
<span class="line"><span>    String: &quot;[0-9]+&quot;</span></span>
<span class="line"><span>  Arg 2:</span></span>
<span class="line"><span>    Identifier: str</span></span></code></pre></div><h3 id="regex-findall-pattern-str" tabindex="-1"><code>regex.findAll(pattern, str)</code> <a class="header-anchor" href="#regex-findall-pattern-str" aria-label="Permalink to &quot;\`regex.findAll(pattern, str)\`&quot;">​</a></h3><div class="language-text vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>Call:</span></span>
<span class="line"><span>  Callee:</span></span>
<span class="line"><span>    Member:</span></span>
<span class="line"><span>      Object:</span></span>
<span class="line"><span>        Identifier: regex</span></span>
<span class="line"><span>      Member:</span></span>
<span class="line"><span>        Identifier: findAll</span></span>
<span class="line"><span>  Arg 1:</span></span>
<span class="line"><span>    Identifier: pattern</span></span>
<span class="line"><span>  Arg 2:</span></span>
<span class="line"><span>    Identifier: str</span></span></code></pre></div><h2 id="module-structure" tabindex="-1">Module Structure <a class="header-anchor" href="#module-structure" aria-label="Permalink to &quot;Module Structure&quot;">​</a></h2><div class="language-text vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark vp-code" tabindex="0"><code><span class="line"><span>regex</span></span>
<span class="line"><span>├── match(pattern, str)      — test match</span></span>
<span class="line"><span>├── find(pattern, str)       — first match</span></span>
<span class="line"><span>├── findAll(pattern, str)    — all matches</span></span>
<span class="line"><span>├── replace(pattern, str, r, global?) — replace</span></span>
<span class="line"><span>└── split(pattern, str)      — split by pattern</span></span></code></pre></div>`,8)])])}const m=s(p,[["render",r]]);export{h as __pageData,m as default};
