
import HomePage from '../pages/home.jsx';
import AboutPage from '../pages/about.jsx';
import ScanPage from '../pages/scan.jsx';
import WhitelistPage from '../pages/whitelist.jsx';

import DynamicRoutePage from '../pages/dynamic-route.jsx';
import NotFoundPage from '../pages/404.jsx';

var routes = [
  {
    path: '/',
    component: HomePage,
  },
  {
    path: '/about/',
    component: AboutPage,
  },
  {
    path: '/scan/',
    async: function (routeTo, routeFrom, resolve, reject) {
      // Router instance
      var router = this;

      // App instance
      var app = router.app;

      // Show Preloader
      app.preloader.show();

      // User ID from request
      var userId = routeTo.params.userId;

      // Simulate Ajax Request
      setTimeout(function () {
        // We got user data from request
        var scan = {
          result: [
            {
              address: `ahoj1`,
              name: `bla11`,
            },
            {
              address: `ahoj2`,
              name: `bla2`,
            },
            {
              address: `ahoj3`,
              name: `bla3`,
            },
          ]
        };
        // Hide Preloader
        app.preloader.hide();

        // Resolve route to load page
        resolve(
          {
            component: ScanPage,
          },
          {
            context: {
              scan: scan,
            }
          }
        );
      }, 1000);
    },

  },
  {
    path: '/whitelist/',
    async: function (routeTo, routeFrom, resolve, reject) {
      // Router instance
      var router = this;

      // App instance
      var app = router.app;

      // Show Preloader
      app.preloader.show();

      // User ID from request
      var userId = routeTo.params.userId;

      // Simulate Ajax Request
      setTimeout(function () {
        // We got user data from request
        var list = {
          result: [
            {
              address: `ahoj1`,
            },
            {
              address: `ahoj2`,
            },
            {
              address: `ahoj3`,
            },
          ]
        };
        // Hide Preloader
        app.preloader.hide();

        // Resolve route to load page
        resolve(
          {
            component: WhitelistPage,
          },
          {
            context: {
              list: list,
            }
          }
        );
      }, 1000);
    },

  },
  {
    path: '/dynamic-route/blog/:blogId/post/:postId/',
    component: DynamicRoutePage,
  },
  {
    path: '(.*)',
    component: NotFoundPage,
  },
];

export default routes;
