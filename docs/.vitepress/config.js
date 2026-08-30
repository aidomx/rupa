import { defineConfig } from 'vitepress';

export default defineConfig({
  base: '/rupa/',
  title: 'Rupa',
  description: 'A general-purpose programming language built from scratch.',
  ignoreDeadLinks: true,

  head: [
    ['meta', { name: 'theme-color', content: '#c8a2ff' }],
    ['meta', { name: 'og:type', content: 'website' }],
    ['meta', { name: 'og:title', content: 'Rupa Documentation' }]
  ],

  themeConfig: {
    logo: false,

    nav: [
      { text: 'Home', link: '/' },
      { text: 'Syntax', link: '/syntax/' },
      { text: 'Grammar', link: '/grammar/' }
    ],

    sidebar: {
      '/syntax/': [
        {
          text: 'Syntax Reference',
          items: [
            { text: 'Overview', link: '/syntax/' },
            { text: 'Syntax Basics', link: '/syntax/syntax' },
            { text: 'Literal', link: '/syntax/literal' },
            { text: 'Expression', link: '/syntax/expression' },
            { text: 'Assignment', link: '/syntax/assignment' },
            { text: 'Update', link: '/syntax/update' },
            { text: 'Fallback', link: '/syntax/fallback' }
          ]
        },
        {
          text: 'Data',
          items: [
            { text: 'String Methods', link: '/syntax/string' },
            { text: 'Array', link: '/syntax/array' },
            { text: 'Object', link: '/syntax/object' },
            { text: 'Struct', link: '/syntax/struct' }
          ]
        },
        {
          text: 'Function',
          items: [
            { text: 'Function', link: '/syntax/function' },
            { text: 'Call', link: '/syntax/call' },
            { text: 'Return', link: '/syntax/return' }
          ]
        },
        {
          text: 'Control Flow',
          items: [
            { text: 'If', link: '/syntax/if' },
            { text: 'Case', link: '/syntax/case' },
            { text: 'Loop', link: '/syntax/loop' },
            { text: 'Control', link: '/syntax/control' },
            { text: 'Block', link: '/syntax/block' }
          ]
        },
        {
          text: 'Asynchronous',
          items: [
            { text: 'Async', link: '/syntax/async' },
            { text: 'Then', link: '/syntax/then' }
          ]
        },
        {
          text: 'Program',
          items: [
            { text: 'Main Entry Point', link: '/syntax/main' },
            { text: 'View Module', link: '/syntax/view' },
            { text: 'Module', link: '/syntax/module' },
            { text: 'Import', link: '/syntax/import' },
            { text: 'Export', link: '/syntax/export' },
            { text: 'Annotation', link: '/syntax/annotation' },
            { text: 'Print', link: '/syntax/print' }
          ]
        }
      ],

      '/grammar/': [
        {
          text: 'Grammar Reference',
          items: [
            { text: 'Overview', link: '/grammar/' },
            { text: 'Grammar', link: '/grammar/grammar' },
            { text: 'Literal', link: '/grammar/literal' },
            { text: 'Expression', link: '/grammar/expression' },
            { text: 'Assignment', link: '/grammar/assignment' },
            { text: 'Update', link: '/grammar/update' },
            { text: 'Fallback', link: '/grammar/fallback' }
          ]
        },
        {
          text: 'Data',
          items: [
            { text: 'Array', link: '/grammar/array' },
            { text: 'Object', link: '/grammar/object' },
            { text: 'Struct', link: '/grammar/struct' },
            { text: 'Member', link: '/grammar/member' }
          ]
        },
        {
          text: 'Function',
          items: [
            { text: 'Function', link: '/grammar/function' },
            { text: 'Call', link: '/grammar/call' },
            { text: 'Return', link: '/grammar/return' }
          ]
        },
        {
          text: 'Control Flow',
          items: [
            { text: 'If', link: '/grammar/if' },
            { text: 'Case', link: '/grammar/case' },
            { text: 'Loop', link: '/grammar/loop' },
            { text: 'Control', link: '/grammar/control' },
            { text: 'Block', link: '/grammar/block' }
          ]
        },
        {
          text: 'Asynchronous',
          items: [
            { text: 'Async', link: '/grammar/async' },
            { text: 'Then', link: '/grammar/then' }
          ]
        },
        {
          text: 'Program',
          items: [
            { text: 'Module', link: '/grammar/module' },
            { text: 'Import', link: '/grammar/import' },
            { text: 'Export', link: '/grammar/export' },
            { text: 'Annotation', link: '/grammar/annotation' },
            { text: 'Print', link: '/grammar/print' }
          ]
        }
      ],

      '/': [
        {
          text: 'Guide',
          items: [
            { text: 'About', link: '/about' },
            { text: 'Vision', link: '/vision' },
            { text: 'Mission', link: '/mission' },
            { text: 'Contributing', link: '/instruction' },
            { text: 'Project Structure', link: '/structure' }
          ]
        }
      ]
    },

    socialLinks: [{ icon: 'github', link: 'https://github.com/aidomx/rupa' }],

    editLink: {
      pattern: 'https://github.com/aidomx/rupa/edit/main/docs/:path',
      text: 'Edit this page on GitHub'
    },

    footer: {
      message: 'Released under the MIT License.',
      copyright: '© 2024 Rupa'
    },

    search: {
      provider: 'local'
    }
  },

  markdown: {
    lineNumbers: true,
    theme: 'github-dark',
    // Alias 'rupa' language to JavaScript for syntax highlighting
    config: md => {
      // Override the fence renderer to map 'rupa' to 'js'
      const defaultFence = md.renderer.rules.fence.bind(md.renderer.rules);
      md.renderer.rules.fence = (tokens, idx, options, env, self) => {
        const token = tokens[idx];
        if (token.info === 'rupa') {
          token.info = 'js';
        }
        return defaultFence(tokens, idx, options, env, self);
      };
    }
  }
});
