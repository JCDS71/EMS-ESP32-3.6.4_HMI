import { defineConfig } from 'vite';
import viteTsconfigPaths from 'vite-tsconfig-paths';
import preact from '@preact/preset-vite';
import viteImagemin from 'vite-plugin-imagemin';
import { visualizer } from 'rollup-plugin-visualizer';

export default defineConfig(({ command, mode }) => {
  if (command === 'serve') {
    console.log('Preparing for standalone build with server, mode=' + mode);
    return {
      plugins: [preact(), viteTsconfigPaths()],
      server: {
        open: true,
        port: mode == 'production' ? 4173 : 3000,
        proxy: {
          '/rest': 'http://localhost:3080',
          '/api': {
            target: 'http://localhost:3080',
            changeOrigin: true,
            secure: false
          },
          '/es': {
            target: 'http://localhost:3080',
            changeOrigin: true,
            secure: false
          }
        }
      }
    };
  }

  if (command === 'build' && mode === 'hosted') {
    return {
      plugins: [preact(), viteTsconfigPaths()],
      build: {
        chunkSizeWarningLimit: 1024
      }
    };
  }

  // production build, both for hosted and building the firmware
  if (command === 'build') {
    return {
      plugins: [
        preact(),
        viteTsconfigPaths(),
        {
          ...viteImagemin({
            verbose: false,
            gifsicle: {
              optimizationLevel: 7,
              interlaced: false
            },
            optipng: {
              optimizationLevel: 7
            },
            mozjpeg: {
              quality: 20
            },
            pngquant: {
              quality: [0.8, 0.9],
              speed: 4
            },
            svgo: {
              plugins: [
                {
                  name: 'removeViewBox'
                },
                {
                  name: 'removeEmptyAttrs',
                  active: false
                }
              ]
            }
          }),
          enforce: 'pre'
        },
        visualizer({
          template: 'treemap', // or sunburst
          open: false,
          gzipSize: true,
          brotliSize: true,
          filename: 'analyse.html' // will be saved in project's root
        })
      ],

      build: {
        // target: 'es2022',
        chunkSizeWarningLimit: 1024,
        minify: 'terser',
        terserOptions: {
          compress: {
            passes: 4,
            arrows: true,
            drop_console: true,
            drop_debugger: true,
            sequences: true
          },
          mangle: {
            // toplevel: true
            // module: true
            // properties: {
            //   regex: /^_/
            // }
          },
          ecma: 5,
          enclose: false,
          keep_classnames: false,
          keep_fnames: false,
          ie8: false,
          module: false,
          nameCache: null,
          safari10: false,
          toplevel: false
        }
      }
    };
  }
});
