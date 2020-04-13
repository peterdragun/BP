import React from 'react';
import {
  Page,
  Navbar,
  List,
  Button,
  Card,
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
      list: props.f7route.context.list,
      succPopupOpened: false,
      errorPopupOpened: props.f7route.context.errorPopup,
      message: props.f7route.context.message,
    };
  }
  render() {
    const list = this.state.list;
    return (
      <Page>
        <Navbar title="Whitelist" backLink="Back" />
        <List>
          {list.result.map((device, index) => (
            <Card key={index}>
              <CardHeader>{`Address: ${device.address}`}</CardHeader>
              <CardFooter>
                <Link></Link>
                <Button fill raised color="red" onClick={() => this.handleClick(device.address.replace(/:/g,''))}>Remove</Button>
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
    if (typeof localStorage.ip == 'undefined'){
      this.setState({ errorPopupOpened : true, message: "Please connect click on 'Get IP address' button on Main page" })
      return;
    }
    axios({
      method: 'post',
      // url: 'http://esp-home.local/ble/device/remove',
      url: 'http://' + localStorage.ip + '/ble/device/remove',
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
}