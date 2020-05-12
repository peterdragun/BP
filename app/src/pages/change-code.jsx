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
      popupOpened: false,
      popupTitle: "",
      code: "",
      new_code: "",
      message: "",
    };
  }
  render() {
    return (
      <Page>
        <Navbar title="Alarm Code" backLink="Back"/>
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
        <Popup opened={this.state.popupOpened} onPopupClosed={() => this.setState({popupOpened : false})}>
          <Page>
            <Navbar title={this.state.popupTitle}>
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
      this.setState({ popupTitle: "Error", popupOpened : true, message: "Please click on 'Find main unit' button on Setup page" })
      return;
    }
    axios({
      method: 'post',
      url: 'http://' + localStorage.ip + '/code/change',
      timeout: 3000,
      data: {
        code: this.state.code,
        new_code: this.state.new_code,
      }
    }).then(response => {this.setState({ popupTitle: "Success", popupOpened : true, message: response.data })}, 
    error => {
      console.log(error);
      var message = "Timeout"
      if (error.response){
        message = error.response.data
      }
      this.setState({ popupTitle: "Error", popupOpened : true, message: message })
    });
  }
}