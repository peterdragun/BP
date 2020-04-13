import React from 'react';
import {
  Page,
  Navbar,
  NavLeft,
  NavTitle,
  NavTitleLarge,
  Link,
  BlockTitle,
  List,
  ListItem,
  Block,
  Button,
  Popup,
  NavRight,
} from 'framework7-react';
import axios from 'axios'

export default class Home extends React.Component {
  constructor(props) {
    super(props);

    this.state = {
      popupOpened: false,
      errorPopupOpened: false,
      message: "",
    }
  }
  render() {
    return (
      <Page name="home">
        <Navbar sliding={false} large>
          <NavLeft>
            <Link iconIos="f7:menu" iconAurora="f7:menu" iconMd="material:menu" panelOpen="left" />
          </NavLeft>
          <NavTitle sliding>Home security</NavTitle>
          <NavTitleLarge>Home security</NavTitleLarge>
        </Navbar>

        <Block>
          <Button fill onClick={() => this.handleClick()}>Activate</Button>
        </Block>

        <Block>
          <Button fill disabled={typeof localStorage.ip !== 'undefined'} onClick={() => this.testBT()}>Get IP address</Button>
        </Block>

        <BlockTitle>Navigation</BlockTitle>
        <List>
          <ListItem link="/scan/" title="Scan"/>
          <ListItem link="/whitelist/" title="Whitelist"/>
          <ListItem link="/change-code/" title="Change alarm code"/>
          <ListItem link="/about/" title="About"/>
        </List>

        <Popup opened={this.state.succPopupOpened} onPopupClosed={() => this.setState({succPopupOpened : false})}>
          <Page>
            <Navbar title="Success">
              <NavRight>
                <Link popupClose>Close</Link>
              </NavRight>
            </Navbar>
            <Block>
              <p>{this.state.message}</p>
            </Block>
          </Page>
        </Popup>
        <Popup opened={this.state.errorPopupOpened} onPopupClosed={() => this.setState({errorPopupOpened : false})}>
          <Page>
            <Navbar title="Error">
              <NavRight>
                <Link popupClose>Close</Link>
              </NavRight>
            </Navbar>
            <Block>
              <p>{this.state.message}</p>
            </Block>
          </Page>
        </Popup>
      </Page>
    );
  }
  handleClick () {
    if (typeof localStorage.ip == 'undefined'){
      this.setState({ errorPopupOpened : true, message: "Please click on 'Get IP address' button" })
      return;
    }
    axios({
      method: 'get',
      // url: 'http://esp-home.local/system/arm', // wont work on android, pls google
      url: 'http://' + localStorage.ip + '/system/arm',
      timeout: 3000
    }).then(response => {this.setState({ succPopupOpened : true, message: response.data })}, 
    error => {
      console.error(error);
      var message = "Timeout"
      if (error.response){
        message = error.response.data
      }
      this.setState({ errorPopupOpened : true, message: message })
    });
  }
  async testBT(){
    var state = this;
    
    cordova.plugins.diagnostic.requestLocationAuthorization(status => {console.error(status)}, error => {console.error(error)});
    cordova.plugins.diagnostic.hasBluetoothLESupport(function(supported){
      console.log("Bluetooth LE is " + (supported ? "supported" : "unsupported"));
    }, error => {handle_error(error)});
    
    new Promise(function (resolve) {
      bluetoothle.initialize(resolve, { request: true, statusReceiver: false });
    }).then(initializeSuccess, error => {handle_error(error)});
    
    function initializeSuccess(result) {
      if (result.status === "enabled") {
        log("Bluetooth is enabled.");
        state.props.f7router.app.preloader.show();
        startScan();
      }
      else {
        handle_error({message: "Bluetooth is not enabled"});
        log(result);
      }
    }

  function log(msg) {
    if (typeof msg === "object") {
      msg = JSON.stringify(msg, null, "  ");
    }
    console.log(msg);
  }

  function handle_error(error){
    log(error);
    state.props.f7router.app.preloader.hide();
    state.setState({ errorPopupOpened : true, message: error.message })
  }

  function startScan() {
    log("Starting scan for devices...", "status");
    if (window.cordova.platformId === "windows") {
      bluetoothle.retrieveConnected(bluetoothle.stopScan({}, error => {log(error)}), error => {console.error(error)}, {});
    }
    else {
      setTimeout(function(){
        bluetoothle.startScan(startScanSuccess, error => {
          error(error);
          bluetoothle.stopScan(
            function() {
              log("end of scan not succ");
            }, error => {log(error)}
          );
        }, { services: [] });
      }, 3000);
    }
  }

  function startScanSuccess(result) {
    log("startScanSuccess(" + result.status + ")");
    if (result.status === "scanStarted") {
        log("Scanning for devices (will continue to scan until you select a device)...", "status");
    }else if (result.status === "scanResult") {
      log(result)
      if (result.name == "ESP_main_unit"){
        bluetoothle.stopScan(
          function() {
            log("end of scan succ");
          }, error => {log(error)}
          );
          new Promise(function (resolve, reject) {
            bluetoothle.connect(resolve, reject, { address: result.address });
          }).then(connectSuccess(result.address), error => {log(error)});
      }
    }
  }

  function connectSuccess(address) {
    new Promise(function (resolve, reject) {
      bluetoothle.discover(resolve, reject,
          { address: address });
    }).then(response => {readService(address, response);}, error => {error.message = error.message + ". Please try again.", handle_error(error)});
  }

  async function readService(address, response) {
    var serviceUuid = "1BA2ECFF-FFCE-4B4E-8562-78F5DCF950B3"
    await new Promise(function (resolve, reject) {
      bluetoothle.read(resolve, reject, { address: address, service: serviceUuid, characteristic: serviceUuid });
    }).then(succ => {
      var arr = bluetoothle.encodedStringToBytes(succ.value);
      var ip = ""
      for (let element of arr) {
        ip = ip + '.' + element;
      }
      localStorage.ip = ip.substr(1);
      log(localStorage.ip)
      
    }, error => handle_error(error));
    bluetoothle.close(success => {log(success)},error => {handle_error(error)},{ address: address })
    state.props.f7router.app.preloader.hide();
    state.setState({ succPopupOpened : true, message: "IP address was successfully stored" })
    return;
  }
}

}