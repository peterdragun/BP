import React from 'react';
import { Page, Navbar, Block, BlockTitle } from 'framework7-react';

export default () => (
  <Page>
    <Navbar title="About" backLink="Back" />
    <BlockTitle>About Application</BlockTitle>
    <Block strong>
      <p>Application is used for setting and managing Smart home security system. System is based on ESP32 boards. Main comunication is done by BLE and WiFi.</p>
      <p>This application was developed as part of my Bachelor Thesis.</p>
      <p>Author: Peter Dragúň</p>
      <p>For more informations feel free to contact me.</p>
    </Block>
  </Page>
);