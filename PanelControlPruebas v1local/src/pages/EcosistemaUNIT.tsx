import { Link } from "react-router-dom";
import HeaderMenu from "../components/HeaderMenu";
import Footer from "../components/Footer";

const buttons = [

  { label: "TEST UE0082 Módulo Relé", to: "/ModuloRele", image: "/img/UE0082 ModuloRele.jpg", protected: false},
  { label: "TEST UE0088 Módulo Buzzer", to: "/ModuloBuzzer", image: "/img/UE0088 ModuloBuzzer.jpg", protected: true},
  { label: "TEST UE0065 DRV2605 Haptic Motor", to: "/HapticMotor", image: "/img/UE0065 HapticMotor.jpg", protected: true},


];

const handleClick = (to: string, protectedButton?: boolean) => {
  if (protectedButton) {
    const password = prompt("Ingrese la contraseña:");
    if (password !== "2021640353") {
      alert("Contraseña incorrecta");
      return;
    }
  }
  window.location.href = to;
};



export default function EcosistemaUNIT() {
  return (
    <div style={{ minHeight: "100vh", position: "relative" }}>
      
      {/* Header fijo */}
      <div style={{ position: "fixed", top: 0, left: 0, width: "100%", height: "60px", zIndex: 100 }}>
        <HeaderMenu />
      </div>

      {/* Contenedor principal centrado */}
      <div
        style={{
          display: "flex",
          flexDirection: "column",
          alignItems: "center",
          width: "606px",
          paddingTop: "160px", // espacio para el header
          paddingBottom: "700px", // espacio para el footer
          paddingRight: "0px", // espacio para el footer
          backgroundColor: "#f3f3f3ff",  // gris de fondo
        }}
      >
        <h1 style={{ marginBottom: "30px", 
            textAlign: "center", 
            paddingRight: "0px",
            color: "#0c0c0cff"}}>Ecosistema de UNIT</h1>

      <div
        style={{
          display: "grid",
          gridTemplateColumns: "repeat(3, 1fr)", // siempre 3 columnas
          rowGap: "20px",                        // separación vertical entre filas
          columnGap: "0px",                     // separación horizontal mínima
          width: "650px",                        // igual al ancho de tu header
          margin: "0 auto",                      // centra el grid en pantalla
          paddingLeft: "30px",
          paddingRight: "30px",
        }}
      >

      {buttons.map((btn, index) => (
        <div
          key={index}
          onClick={() => handleClick(btn.to, btn.protected)}
          style={{
            display: "flex",
            flexDirection: "column",
            justifyContent: "center",
            alignItems: "center",
            width: "100px",
            height: "180px",
            borderRadius: "8px",
            backgroundColor: "#ffffffff",
            boxShadow: "0 20px 12px rgba(0,0,0,0.1)",
            color: "#000000ff",
            fontWeight: "bold",
            textDecoration: "none",
            transition: "transform 0.3s, box-shadow 0.3s",
            padding: "5px",
            textAlign: "center",
            cursor: "pointer", 
          }}
          onMouseEnter={e => {
            e.currentTarget.style.transform = "scale(1.05)";
            e.currentTarget.style.boxShadow = "0 8px 20px rgba(0,0,0,0.3)";
          }}
          onMouseLeave={e => {
            e.currentTarget.style.transform = "scale(1)";
            e.currentTarget.style.boxShadow = "0 20px 12px rgba(0,0,0,0.1)";
          }}
        >
          <img
            src={btn.image}
            alt={btn.label}
            style={{ width: "100px", height: "100px", objectFit: "contain", marginBottom: "2px" }}
          />
          <span>{btn.label}</span>
        </div>
      ))}

        </div>


    <div
      style={{
        display: "flex",
        justifyContent: "center",
        alignItems: "center",
        //height: "100vh", // ocupa toda la altura de la ventana
        marginTop: "30px",
        marginBottom: "0px",
        width: "100%",   // ocupa todo el ancho
        //backgroundColor: "#f9f9f9", // opcional, un fondo claro
      }}
    >
      <Link
        to="/"
        style={{
          display: "inline-block",
          padding: "16px 32px",
          background: "linear-gradient(135deg, #667eea, #764ba2)",
          color: "white",
          fontSize: "20px",
          fontWeight: "bold",
          textDecoration: "none",
          borderRadius: "12px",
          boxShadow: "0 4px 12px rgba(0,0,0,0.25)",
          transition: "all 0.3s ease",
          textAlign: "center",
        }}
        onMouseOver={(e) => {
          e.currentTarget.style.background =
            "linear-gradient(135deg, #5a67d8, #6b46c1)";
          e.currentTarget.style.transform = "scale(1.05)";
          e.currentTarget.style.boxShadow = "0 6px 16px rgba(0,0,0,0.35)";
        }}
        onMouseOut={(e) => {
          e.currentTarget.style.background =
            "linear-gradient(135deg, #667eea, #764ba2)";
          e.currentTarget.style.transform = "scale(1)";
          e.currentTarget.style.boxShadow = "0 4px 12px rgba(0,0,0,0.25)";
        }}
      >
        Volver al menú principal
      </Link>
    </div>





    {/* Footer fijo */}
    <div style={{ position: "fixed", bottom: 0, left: 0, width: "100%", height: "60px", zIndex: 100 }}>
      <Footer />
    </div>






      </div>








    </div>
  );
}
