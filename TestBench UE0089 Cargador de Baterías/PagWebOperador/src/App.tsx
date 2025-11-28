import Header from "./components/Header";
import Operador from "./components/PanelOpeador";
import Footer from "./components/Footer";

function App() {
  return (
    <div style={{ display: "flex", flexDirection: "column", minHeight: "100vh" }}>
      <Header />
      <Operador />
      <Footer />
    </div>
  );
}

export default App;




  

