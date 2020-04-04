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
      errorPopupOpened: false,
      message: "",
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
              {/* <CardContent>{`Name: ${device.name}`}</CardContent> */}
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
    axios({
      method: 'post',
      // url: 'http://esp-home.local/ble/device/remove',
      url: 'http://192.168.1.45/ble/device/remove',
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