import React from 'react';
import {
  Page,
  Navbar,
  List,
  Link,
  Button,
  Popup,
  NavRight,
  Block,
  ListItem,
  ListInput
} from 'framework7-react';
import axios from 'axios'

export default class extends React.Component {
  constructor(props) {
    super(props);

    this.state = {
      succPopupOpened: false,
      errorPopupOpened: false,
      code: "",
      new_code: "",
      message: "",
    };
  }
  render() {
    return (
      <Page>
        <Navbar title="Alarm Code" backLink="Back"/>
        <Block>
          <List>
            <ListInput
              type="number"
              label="Current code"
              required
              info="Default code is 123456"
              min="0"
              max="9223372036854775807" // 64bit max val
              placeholder="123456"
              value={this.state.code}
              onChange={(event) => this.setState({code: event.target.value})}
            />
            <ListInput
              type="number"
              label="New code"
              required
              min="0"
              max="9223372036854775807" // 64bit max val
              placeholder="1234"
              value={this.state.new_code}
              onChange={(event) => this.setState({new_code: event.target.value})}
            />
            <ListItem>
              <Button fill onClick={() => this.handleClick()}>Change</Button>
            </ListItem>
          </List>
        </Block>
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
      this.setState({ errorPopupOpened : true, message: "Please connect click on 'Get IP address' button on Main page" })
      return;
    }
    axios({
      method: 'post',
      // url: 'http://esp-home.local/code/change',
      url: 'http://' + localStorage.ip + '/code/change',
      timeout: 3000,
      data: {
        code: this.state.code,
        new_code: this.state.new_code,
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