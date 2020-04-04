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
    };
  }
  render() {
    const list = this.state.list;
    return (
      <Page>
        <Navbar title="Scan" backLink="Back" />
        <List>
          {list.result.map((device, index) => (
            <Card key={index}>
              <CardHeader>{`Address: ${device.address}`}</CardHeader>
              {/* <CardContent>{`Name: ${device.name}`}</CardContent> */}
              <CardFooter>
                <Link></Link>
                <Button fill raised color="red" onClick={() => this.handleClick(device.address)}>Remove</Button>
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
              <p>Device was successfully removed from whitelist.</p>
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
              <p>Error ocured.</p>
            </Block>
          </Page>
        </Popup>
      </Page>
    );
  }
  handleClick (address) {
    axios({
      method: 'post',
      url: 'http://esp-home.local/ble/device/remove',
      data: {
        address: address,
      }
    }).then(response => {this.setState({ succPopupOpened : true })}, error => { console.log(error), this.setState({ errorPopupOpened : true })} );
  }
}