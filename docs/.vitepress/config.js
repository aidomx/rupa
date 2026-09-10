import { defineConfig } from 'vitepress'

export default defineConfig({
  base: '/rupa/',
  title: 'Rupa',
  description: 'One Language, Many Ways to Speak',
  
  head: [
    ['link', { rel: 'icon', type: 'image/svg+xml', href: '/rupa/logo.svg' }]
  ],

  themeConfig: {
    nav: [
      { text: 'Home', link: '/' },
      { text: 'Syntax', link: '/syntax/' },
      { text: 'Grammar', link: '/grammar/' },
      { text: 'Modules', link: '/modules/syntax/' },
      { text: 'TODO', link: '/TODO' }
    ],

    sidebar: {
      '/syntax/': [
        {
          text: 'Syntax',
          items: [
            { text: 'Index', link: '/syntax/' },
            { text: 'Main', link: '/syntax/main' },
            { text: 'Literal', link: '/syntax/literal' },
            { text: 'Print', link: '/syntax/print' },
            { text: 'Assignment', link: '/syntax/assignment' },
            { text: 'Expression', link: '/syntax/expression' },
            { text: 'String', link: '/syntax/string' },
            { text: 'Array', link: '/syntax/array' },
            { text: 'Object', link: '/syntax/object' },
            { text: 'If/Else', link: '/syntax/if' },
            { text: 'Block', link: '/syntax/block' },
            { text: 'Function', link: '/syntax/function' },
            { text: 'Return', link: '/syntax/return' },
            { text: 'Loop', link: '/syntax/loop' },
            { text: 'Case', link: '/syntax/case' },
            { text: 'Call', link: '/syntax/call' },
            { text: 'Struct', link: '/syntax/struct' },
            { text: 'Annotation', link: '/syntax/annotation' },
            { text: 'Update', link: '/syntax/update' },
            { text: 'Control', link: '/syntax/control' },
            { text: 'Fallback', link: '/syntax/fallback' },
            { text: 'Then', link: '/syntax/then' },
            { text: 'Async', link: '/syntax/async' },
            { text: 'Module', link: '/syntax/module' },
            { text: 'Import', link: '/syntax/import' },
            { text: 'Export', link: '/syntax/export' }
          ]
        }
      ],
      '/grammar/': [
        {
          text: 'Grammar',
          items: [
            { text: 'Index', link: '/grammar/' },
            { text: 'Module', link: '/grammar/module' },
            { text: 'Import', link: '/grammar/import' },
            { text: 'Export', link: '/grammar/export' },
            { text: 'Assignment', link: '/grammar/assignment' },
            { text: 'Expression', link: '/grammar/expression' },
            { text: 'Function', link: '/grammar/function' },
            { text: 'If', link: '/grammar/if' },
            { text: 'Block', link: '/grammar/block' },
            { text: 'Loop', link: '/grammar/loop' },
            { text: 'Case', link: '/grammar/case' },
            { text: 'Call', link: '/grammar/call' },
            { text: 'Struct', link: '/grammar/struct' },
            { text: 'Annotation', link: '/grammar/annotation' },
            { text: 'Async', link: '/grammar/async' },
            { text: 'HTTP', link: '/grammar/http' }
          ]
        }
      ],
      '/modules/': [
        {
          text: 'Modules',
          items: [
            { text: 'Math', link: '/modules/syntax/math' },
            { text: 'OS', link: '/modules/syntax/os' },
            { text: 'IO', link: '/modules/syntax/io' },
            { text: 'JSON', link: '/modules/syntax/json' },
            { text: 'String', link: '/modules/syntax/string' },
            { text: 'Thread', link: '/modules/syntax/thread' },
            { text: 'HTTP', link: '/modules/syntax/http' }
          ]
        }
      ]
    },

    socialLinks: [
      { icon: 'github', link: 'https://github.com/aidomx/rupa' }
    ],

    footer: {
      message: 'Released under the MIT License.',
      copyright: '© 2026 Rupa Language'
    }
  }
})
