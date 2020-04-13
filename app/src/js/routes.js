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
      if (typeof localStorage.ip == 'undefined'){
        resolve(
          {
            component: ScanPage,
          },
          {
            context: {
              scan: {result: []},
              errorPopup: true,
              message: "Please click on 'Get IP address' button on Main page",
            }
          }
        );
      }else{
        this.app.preloader.show();
        axios({
          method: 'get',
          // url: 'http://esp-home.local/ble/scan',
          url: 'http://' + localStorage.ip + '/ble/scan',
          timeout: 8000
        }).then(response => {
          console.log(response);
          var array = removeDuplicates(x => x.address, response.data);
          this.app.preloader.hide();
          resolve(
            {
              component: ScanPage,
            },
            {
              context: {
                scan: {result: array},
                errorPopup: false,
                message: "",
              }
            }
          );
        }, error => {
          console.log(error),
          this.app.preloader.hide();
          resolve(
            {
              component: ScanPage,
            },
            {
              context: {
                scan: {result: []},
                errorPopup: true,
                message: error.message,
              }
            }
          );
        });
      }
      
    },

  },
  {
    path: '/whitelist/',
    async: function (routeTo, routeFrom, resolve, reject) {
      var list = {result: []}
      if (typeof localStorage.ip == 'undefined'){
        resolve(
          {
            component: WhitelistPage,
          },
          {
            context: {
              list: {result: []},
              message: "Please click on 'Get IP address' button on Main page",
              errorPopup: true,
            }
          }
        );
      }else{
        this.app.preloader.show();
        axios({
          method: 'get',
          // url: 'http://esp-home.local/ble/scan',
          url: 'http://' + localStorage.ip + '/ble/device/list',
          timeout: 3000
        }).then(response => {
          this.app.preloader.hide();
          resolve(
            {
              component: WhitelistPage,
            },
            {
              context: {
                list: {result: response.data},
                message: "",
                errorPopup: false,
              }
            }
          );
        }, error => {
          this.app.preloader.hide();
          console.log(error),
          resolve(
            {
              component: WhitelistPage,
            },
            {
              context: {
                list: {result: []},
                message: error.message,
                errorPopup: true,
              }
            }
          );
        });
      }
      
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
