import React from 'react';
import {
  Page,
  Navbar,
  List,
  Button,
  Card,
  CardContent,
  CardFooter,
  CardHeader,
  Link,
  Popup,
  NavRight,
  Block,
} from 'framework7-react';
import axios from 'axios'

export default class extends React.Component {
  constructor(props) {
    super(props);

    this.state = {
      scan: props.f7route.context.scan,
      succPopupOpened: false,
      errorPopupOpened: false,
      message: "",
    };
  }
  render() {
    const scan = this.state.scan;
    return (
      <Page ptr onPtrRefresh={this.reload.bind(this)}>
        <Navbar title="Scan" backLink="Back" />
        <List>
          {scan.result.map((device, index) => (
            <Card key={index}>
              <CardHeader>{`Address: ${device.address}`}</CardHeader>
              <CardContent>{`Name: ${device.name}`}</CardContent>
              <CardFooter>
                <Link></Link>
                <Button fill raised onClick={() => this.handleClick(device.address)}>Add</Button>
              </CardFooter>
            </Card>
          ))}
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
  handleClick (address) {
    axios({
      method: 'post',
      // url: 'http://esp-home.local/ble/device/add',
      url: 'http://192.168.1.45/ble/device/add',
      timeout: 3000,
      data: {
        address: address,
      }
    }).then(response => {this.setState({ succPopupOpened : true, message: response.data })},
    error => {
      console.log(error);
      var message = "Timeout"
      if (error.response){
        message = error.response.data
      }
      this.setState({ errorPopupOpened : true, message: message })
    });
  }
  reload(done){
    var {scan} = this.state;
    axios({
      method: 'get',
      // url: 'http://esp-home.local/ble/scan',
      url: 'http://192.168.1.45/ble/scan',
      timeout: 8000
    }).then(response => {
      console.log(response);
      // var array = removeDuplicates(x => x.address, response.data);
      scan = {result: response.data};
      this.setState({scan});
      done();
    }, error => {
      console.log(error);
      var message = "Timeout"
      if (error.response){
        message = error.response.data
      }
      this.setState({ errorPopupOpened : true, message: message })
      done();
    });
  }
}