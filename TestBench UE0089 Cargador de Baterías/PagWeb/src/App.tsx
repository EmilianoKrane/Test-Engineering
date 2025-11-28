//import { useState } from 'react'
//import reactLogo from './assets/react.svg'
//import viteLogo from '/vite.svg'
//import './App.css'

//import Boton1 from './components/Boton1'

import React from "react";
import Header from "./components/Header";
import Content from "./components/Content";
import Footer from "./components/Footer";


  function App() {

  //const [count, setCount] = useState(0)

    return (
      <div style={{ 
        display: "flex", 
        flexDirection: "column", 
        minHeight: "100vh" 
      }}>
        <Header />
        <Content />
        <Footer />
      </div>
    );
  }

  export default App;






  // Return original
  /*  
  return (
    <>
      <div>
        <a href="https://vite.dev" target="_blank">
          <img src={viteLogo} className="logo" alt="Vite logo" />
        </a>
        <a href="https://react.dev" target="_blank">
          <img src={reactLogo} className="logo react" alt="React logo" />
        </a>
      </div>
      <h1>Hola carambola</h1>
      <div className="card">
        <button onClick={() => setCount((count) => count + 1)}>
          count is {count}
        </button>
        <p>
          Edit <code>src/App.tsx</code> and save to test HMR
        </p>
      </div>
      <p className="read-the-docs">
        Click on the Vite and React logos to learn more
      </p>
    </>
  )
*/



