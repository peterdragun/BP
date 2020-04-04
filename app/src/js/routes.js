
import HomePage from '../pages/home.jsx';
import AboutPage from '../pages/about.jsx';
import ScanPage from '../pages/scan.jsx';
import WhitelistPage from '../pages/whitelist.jsx';
import ChangeCodePage from '../pages/change-code.jsx';

import NotFoundPage from '../pages/404.jsx';
import axios from 'axios'

function removeDuplicates(keyFn, array){
  var trimmedArray = [];
  var values = [];
  var value;

  for(var i = 0; i < array.length; i++) {
    value = array[i]["address"];

    if(values.indexOf(value) === -1) {
      trimmedArray.push(array[i]);
      values.push(value);
    }
  }

  return trimmedArray;
}

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
      var router = this;
      var app = router.app;
      app.preloader.show();

      axios({
        method: 'get',
        // url: 'http://esp-home.local/ble/scan',
        url: 'http://192.168.1.45/ble/scan',
        timeout: 8000
      }).then(response => {
        console.log(response);
        var array = removeDuplicates(x => x.address, response.data);
        var scan = {result: array};
        app.preloader.hide();

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
      }, error => {
        console.log(error),
        app.preloader.hide();
      });
    },

  },
  {
    path: '/whitelist/',
    async: function (routeTo, routeFrom, resolve, reject) {
      var router = this;
      var app = router.app;
      app.preloader.show();
      axios({
        method: 'get',
        // url: 'http://esp-home.local/ble/scan',
        url: 'http://192.168.1.45/ble/device/list',
        timeout: 3000
      }).then(response => {
        var list = {result: response.data};
        app.preloader.hide();
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
      }, error => {
        console.log(error),
        app.preloader.hide();
      });
    },

  },
  {
    path: '/change-code/',
    component: ChangeCodePage,
  },
  {
    path: '(.*)',
    component: NotFoundPage,
  },
];

export default routes;
