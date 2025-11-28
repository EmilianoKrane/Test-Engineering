//import React from "react";
import Header from "../components/HeaderCV";
import Footer from "../components/Footer";
//import React, { useState } from "react";
import  { useState } from "react";
import { Link } from 'react-router-dom';

export default function Config() {

// Estado para la comunicación serie por puerto
const [port, setPort] = useState<SerialPort | null>(null);

// Estados para cada textarea del modo dinámico
const [amplitudA, setAmplitudA] = useState("");
const [anchoA, setAnchoA] = useState("");
const [amplitudB, setAmplitudB] = useState("");
const [anchoB, setAnchoB] = useState("");
const [voltajeMax, setVoltajeMax] = useState("");
const [corrienteMax, setCorrienteMax] = useState("");

// Estados para cada textarea del modo LIST
const [noPasos, setnoPasos] = useState("");
const [incCorriente, setincCorriente] = useState("");
const [incTiempo, setincTiempo] = useState("");
const [nombreArch, setnombreArch] = useState("");
const [corrienteMaxList, setcorrienteMaxList] = useState("");
const [voltajeMaxList, setvoltajeMaxList] = useState("");

// Estados para las lecturas
const [voltajeMes, setVoltajeMes] = useState("Lectura");
const [corrienteMes, setCorrienteMes] = useState("Lectura");
const [potenciaMes, setPotenciaMes] = useState("Lectura");



// Función para “Establecer parámetros” Dinámicos
const SetParamsDIN = () => {

  console.log("Amplitud A:", amplitudA);
  console.log("Ancho A:", anchoA);
  console.log("Amplitud B:", amplitudB);
  console.log("Ancho B:", anchoB);
  console.log("Voltaje Max:", voltajeMax);
  console.log("Corriente Max:", corrienteMax);

  // Enviar en JSON
  sendJSON(
    JSON.stringify({
      Funcion: "DYN",
      Config: {
        Resolution: [parseFloat(amplitudA), parseFloat(anchoA), parseFloat(amplitudB), parseFloat(anchoB)],
        Range: [parseFloat(voltajeMax), parseFloat(corrienteMax)]
      },
      Start: "CFG_ON"
    })
  );
};

// Función para “Establecer parámetros” LIST
const SetParamsLIST = () => {

  console.log("No. pasos:", noPasos);
  console.log("Inc. Corriente:", incCorriente);
  console.log("Inc. Tiempo:", incTiempo);
  console.log("Nombre Archivo:", nombreArch);
  console.log("Corriente Max:", corrienteMaxList);
  console.log("Voltaje Max:", voltajeMaxList);

// Enviar en JSON
sendJSON(
  JSON.stringify({
    Funcion: "LIST",
    Config: {
      Resolution: [parseFloat(noPasos), parseFloat(incCorriente), parseFloat(incTiempo), parseFloat(nombreArch)],
      Range: [parseFloat(corrienteMaxList), parseFloat(voltajeMaxList)]
    },
    Start: "CFG_ON"
  })
);
};


// Función que envía el JSON de medición en Carga Variable 
const medirCarga = async () => {

  // Limpieza de casillas en cada lectura
  setVoltajeMes("")
  setCorrienteMes("")  
  setPotenciaMes("")

  if (!port) return;

  // 1️⃣ Enviar JSON de lectura
  await sendJSON(
    '{"Funcion": "Other", "Config": {"Resolution": [], "Range": []}, "Start": "Read"}'
  );

  // 2️⃣ Leer la respuesta del puerto UART
  if (port.readable) {
    const reader = port.readable.getReader();
    try {
      const { value, done } = await reader.read();
      if (done) return;

      if (value) {
        const text = new TextDecoder().decode(value).trim().split("\n")[0];;
        console.log("Respuesta UART:", text);

        // 3️⃣ Separar los valores por ;
        const partes = text.split(";");
        if (partes.length == 3) {
          setVoltajeMes(partes[0]);
          setCorrienteMes(partes[1]);
          setPotenciaMes(partes[2]);
        }

      }
    } 
    catch (err) {
      console.error("Error leyendo UART:", err);
    } finally {
      reader.releaseLock();
    }
  }
};





const connectCOM = async () => {
  try {
    // pedir puerto al usuario
    const selectedPort: SerialPort = await (navigator as any).serial.requestPort();
    await selectedPort.open({ baudRate: 9600 }); 

    // guardamos el puerto en el estado
    setPort(selectedPort);
    console.log("Puerto COM conectado correctamente");

    // empezamos a leer de la pulsar
    // readFromLoad(selectedPort);
  } 
  catch (err) {
    console.error("Error al conectar con la placa:", err);
  }
};




const sendJSON = async (char: string) => {
  if (port && port.writable) {
    const encoder = new TextEncoder();
    const writer = port.writable.getWriter();
    
    await writer.write(encoder.encode(char + "\r\n"));  
    
    writer.releaseLock();
    console.log("✅ JSON enviado:", char);
  }
};






  return (
    <div
      style={{
        display: "flex", 
        flexDirection: "column", 
        height: "100vh" // ocupa toda la pantalla


      }}
    >

    <div style={{ position: "fixed", top: 0, left: 0, width: "100%", height: "60px", zIndex: 100  }}>
      <Header />
    </div>


      <main 
        style={{ 
        flex: 1,
        overflowY: "auto",
        overflowX: "hidden",
        display: "flex",
        flexDirection: "column",
        alignItems: "center",
        textAlign: "center",
        paddingTop: "180px",    // espacio extra para que el header no tape nada
        paddingBottom: "60px", // espacio extra para footer
        width: "590px", 
        paddingLeft: "10px",   // espacio izquierdo
        paddingRight: "25px",  // espacio derecho

        }}
      >

      <div
        style = {{
          display: "flex",       // alineación en fila
          alignItems: "center",  // centrado vertical
          gap: "15px",           // espacio entre elementos
        }}>
      </div>


        {/* Primera sección: Inicialización de la carga */}
        <div
          style={{
            padding: "10px",
            width: "100%",
            maxWidth: "500px",
            textAlign: "center",
            alignItems: "center"
          }}
        >
          <h2 style={{ marginBottom: "15px" }}>Inicialización de la Carga Variable</h2>

          {/* Botones de inicialización */}
          <div
            style={{
              display: "grid",
              gridTemplateColumns: "repeat(3, 1fr)",
              gap: "20px",
              marginBottom: "20px",
            }}
          >

          <button onClick={connectCOM}
          style={{
              padding: "5px",
              borderRadius: "10px",
              cursor: "pointer",
              flex: 1,
              backgroundColor: "#bababa70"    
            }}
          >Seleccionar puerto COM</button>

          <button
            onClick={() =>
              sendJSON('{"Funcion":"Other","Config":{"Resolution":[],"Range":[]},"Start":"CFG_ON"}')
            }
          style={{
                  padding: "5px",
                  borderRadius: "10px",
                  cursor: "pointer",
                  flex: 1,
                  backgroundColor: "#bababa70"   
                }}
          >
            Encendido
          </button>

          <button
            onClick={() =>
              sendJSON('{"Funcion": "Other", "Config": {"Resolution": [], "Range": []}, "Start": "CFG_OFF"}')
            }
            style={{
                  padding: "5px",
                  borderRadius: "10px",
                  cursor: "pointer",
                  flex: 1,
                  backgroundColor: "#bababa70"   
                }}
          >
            Apagado
          </button>
          </div>
        </div>


      {/* Sección de Modos de Configuración */}

      <div
      style={{
        display: "flex",           // activa el flex
        flexDirection: "row",      // organiza en fila
        gap: "20px",               // espacio entre columnas
        width: "100%",             // ocupa todo el ancho del contenedor padre
        maxWidth: "500px",         // opcional, ancho máximo
        margin: "0 auto",          // centra horizontalmente
        marginBottom: "0px",      // espacio debajo del contenedor
        paddingLeft: "0px",       // espacio izquierdo
        paddingRight: "0px",      // espacio derecho
        boxSizing: "border-box",   // incluye padding en el cálculo
      }}
      >

        {/* Modo de Configuración Dinámico */}
        <div
          style={{
            flex: 1,                 // ocupa la mitad del espacio
            //width: "250px",
            padding: "10px",
            border: "1px solid #ccc", // opcional, para visualizar
            borderRadius: "8px",
            textAlign: "center",      // centra el título
          }}
        >
          <h3>Modo Dinámico</h3>

        {/* Entrada de datos */}
        <div style={{ display: "flex", flexDirection: "column", gap: "10px", marginBottom: "10px" }}>

          <div style={{ display: "flex", justifyContent: "space-between", gap: "10px" }}>
            <span>Amplitud A:</span>
            <textarea 
            value={amplitudA}
            onChange={(e) => setAmplitudA(e.target.value)}
            style={{height: "30px", resize: "none", width: "90px", textAlign: "left" }} />
          </div>

          <div style={{ display: "flex", justifyContent: "space-between", gap: "10px" }}>
            <span>Ancho A:</span>
            <textarea 
            value={anchoA}
            onChange={(e) => setAnchoA(e.target.value)}
            style={{ height: "30px", resize: "none", width: "90px", textAlign: "left" }} />
          </div>

          <div style={{ display: "flex", justifyContent: "space-between", gap: "10px" }}>
            <span>Amplitud B:</span>
            <textarea 
            value={amplitudB}
            onChange={(e) => setAmplitudB(e.target.value)}
            style={{ height: "30px", resize: "none", width: "90px", textAlign: "left" }} />
          </div>
          
          <div style={{ display: "flex", justifyContent: "space-between", gap: "10px" }}>
            <span>Ancho B:</span>
            <textarea 
            value={anchoB}
            onChange={(e) => setAnchoB(e.target.value)}
            style={{ height: "30px", resize: "none", width: "90px", textAlign: "left" }} />
          </div>
        
          <div style={{ display: "flex", justifyContent: "space-between", gap: "10px" }}>
            <span>Voltaje Max:</span>
            <textarea 
            value={voltajeMax}
            onChange={(e) => setVoltajeMax(e.target.value)}
            style={{height: "30px", resize: "none", width: "90px", textAlign: "left" }} />
          </div>

          <div style={{ display: "flex", justifyContent: "space-between", gap: "10px" }}>
            <span>Corriente Max:</span>
            <textarea 
            value={corrienteMax}
            onChange={(e) => setCorrienteMax(e.target.value)}
            style={{ height: "30px", resize: "none", width: "90px", textAlign: "left" }} />
          </div>
        </div>

          {/* Contenedor de botones en fila */}
          <div
            style={{
              display: "flex",
              flexDirection: "row",
              justifyContent: "center",
              gap: "10px",
              marginTop: "10px", // espacio entre título y botones
            }}
          >
            {/* Botón de SET para Datos de Configuración */}
            <button
            onClick={SetParamsDIN}
              style={{
                padding: "5px",
                borderRadius: "5px",
                cursor: "pointer",
                backgroundColor: "#e4e4e490",
              }}
            >
              Establecer Parámetros
            </button>

            {/* Botón de envío de datos/activación */}
            <button
              onClick={() =>
                sendJSON(
                  '{"Funcion":"Other","Config":{"Resolution":[],"Range":[]},"Start":"CFG_ON"}'
                )
              }
              style={{
                padding: "5px",
                borderRadius: "5px",
                cursor: "pointer",
                backgroundColor: "#e4e4e490",
              }}
            >
              Activar
            </button>
          </div>
        </div>


        {/* Modo de Configuración LIST */}
        <div
          style={{
            flex: 1,                 // ocupa la mitad del espacio
            padding: "10px",
            border: "1px solid #ccc", // opcional, para visualizar
            borderRadius: "8px",
            textAlign: "center",      // centra el título
          }}
        >
          <h3>Modo LIST</h3>

        {/* Entrada de datos */}
        <div style={{ display: "flex", flexDirection: "column", gap: "10px", marginBottom: "10px" }}>

          <div style={{ display: "flex", justifyContent: "space-between", gap: "10px" }}>
            <span>No. pasos:</span>
            <textarea 
            value={noPasos}
            onChange={(e) => setnoPasos(e.target.value)}
            style={{ height: "30px", resize: "none", width: "90px", textAlign: "left" }} />
          </div>

          <div style={{ display: "flex", justifyContent: "space-between", gap: "10px" }}>
            <span>Inc. Corriente:</span>
            <textarea 
            value={incCorriente}
            onChange={(e) => setincCorriente(e.target.value)}
            style={{ height: "30px", resize: "none", width: "90px", textAlign: "left" }} />
          </div>

          <div style={{ display: "flex", justifyContent: "space-between", gap: "10px" }}>
            <span>Inc. Tiempo:</span>
            <textarea 
            value={incTiempo}
            onChange={(e) => setincTiempo(e.target.value)}
            style={{ height: "30px", resize: "none", width: "90px", textAlign: "left" }} />
          </div>
          
          <div style={{ display: "flex", justifyContent: "space-between", gap: "10px" }}>
            <span>Nombre Arch:</span>
            <textarea 
            value={nombreArch}
            onChange={(e) => setnombreArch(e.target.value)}style={{height: "30px", resize: "none", width: "90px", textAlign: "left" }} />
          </div>
        
          <div style={{ display: "flex", justifyContent: "space-between", gap: "10px" }}>
            <span>Corriente Max:</span>
            <textarea 
            value={corrienteMaxList}
            onChange={(e) => setcorrienteMaxList(e.target.value)}
            style={{ height: "30px", resize: "none", width: "90px", textAlign: "left" }} />
          </div>

          <div style={{ display: "flex", justifyContent: "space-between", gap: "10px" }}>
            <span>Voltaje Max:</span>
            <textarea 
            value={voltajeMaxList}
            onChange={(e) => setvoltajeMaxList(e.target.value)}
            style={{ height: "30px", resize: "none", width: "90px", textAlign: "left" }} />
          </div>
        </div>



          {/* Contenedor de botones en fila */}
          <div
            style={{
              display: "flex",
              flexDirection: "row",
              justifyContent: "center",
              gap: "10px",
              marginTop: "10px", // espacio entre título y botones
            }}
          >
            {/* Botón de SET para Datos de Configuración */}
            <button
            onClick={SetParamsLIST}
              style={{
                padding: "5px",
                borderRadius: "5px",
                cursor: "pointer",
                backgroundColor: "#e4e4e490",
              }}
            >
              Establecer Parámetros
            </button>

            {/* Botón de envío de datos/activación */}
            <button
              onClick={() =>
                sendJSON(
                  '{"Funcion":"Other","Config":{"Resolution":[],"Range":[]},"Start":"CFG_ON"}'
                )
              }
              style={{
                padding: "5px",
                borderRadius: "5px",
                cursor: "pointer",
                backgroundColor: "#e4e4e490",
              }}
            >
              Activar
            </button>
          </div>
        </div>
      </div>


      {/* Tercera sección: Lectura de datos */}

      <h3>Medición de la Carga Variable </h3>
      
      <div
        style={{
          display: "flex",
          gap: "20px",           // espacio entre columnas
          width: "100%",
          maxWidth: "520px",     // ancho máximo del contenedor
          padding: "10px",
          border: "1px solid #ccc",
          borderRadius: "8px",
          marginBottom: "10px",  // espacio debajo del contenedor
          textAlign: "center",
          justifyContent: "space-between",
          paddingLeft: "0px",       // espacio izquierdo
          paddingRight: "30px",      // espacio derecho
        }}
      >


        {/* Columna 1: Medición Voltaje*/}
        <div 
          style={{ 
            flex: 1, 
            display: "flex", 
            flexDirection: "column", 
            alignItems: "center",     // centra horizontalmente
            paddingLeft: "20px",       // espacio izquierdo
            gap: "5px" }}>

          <span>Voltaje, V</span>

          <textarea
            style={{ flex: 1, resize: "none", 
              width: "100px",
              alignItems: "center",
              height: "20px" }}
              readOnly
              value={voltajeMes}
          />
        </div>

        {/* Columna 2: Medición Corriente */}
        <div 
        style={{ 
          flex: 1, 
          display: "flex", 
          flexDirection: "column", 
          alignItems: "center",     // centra horizontalmente
          gap: "5px" }}>

          <span>Corriente, A</span>

          <textarea
            style={{ flex: 1, resize: "none", 
              width: "100px",
              alignItems: "center",
              height: "20px" }}
              readOnly
              value={corrienteMes}
          />
        </div>

        {/* Columna 3: Medición Potencia */}
        <div 
          style={{ flex: 1, 
          display: "flex", 
          flexDirection: "column", 
          alignItems: "center",     // centra horizontalmente
          gap: "5px" }}>

          <span>Potencia, W</span>

          <textarea
            style={{ flex: 1, resize: "none", 
              width: "100px",
              alignItems: "center",
              height: "20px" }}
              readOnly
              value={potenciaMes}
          />
        </div>

        {/* Botón de Medición */}
        <button
        onClick={medirCarga}
        style={{
                padding: "0px",
                borderRadius: "10px",
                cursor: "pointer",
                height: "40px",
                flex: 1,
                marginTop: "20px",
                backgroundColor: "#bababa70"   
              }}
        >
          Medir
        </button>
      </div>










      {/* Cuarta sección: RESET e IDIOMA */}
        <div
          style={{
            padding: "10px",
            width: "100%",
            maxWidth: "500px",
            textAlign: "center",
          }}
        >

          {/* Botón de RESET */}
          <div
            style={{
              display: "grid",
              gridTemplateColumns: "repeat(3, 1fr)",
              gap: "20px",
              marginBottom: "20px",
            }}
          >

          <button
            onClick={() =>
              sendJSON('{"Funcion": "RST", "Config": {"Resolution": [], "Range": []}, "Start": "CFG_ON"}')
            }
          style={{
                  padding: "5px",
                  borderRadius: "10px",
                  cursor: "pointer",
                  flex: 1,
                  backgroundColor: "#bababa70"   
                }}
          >
            RESET
          </button>

          <button
            onClick={() =>
              sendJSON(' {"Funcion": "LANG", "Config": {"Resolution": [], "Range": []}, "Start": "CFG_ON"}')
            }
            style={{
                  padding: "5px",
                  borderRadius: "10px",
                  cursor: "pointer",
                  flex: 1,
                  backgroundColor: "#bababa70"   
                }}
          >
            Idioma Inglés
          </button>

            <div>
            <Link to="/">Volver al menú</Link>
            </div>



          </div>
        </div>







      </main>

    <div style={{ position: "fixed", bottom: 0, left: 0, width: "100%", height: "60px", zIndex: 100  }}>
      <Footer />
    </div>

    </div>
  );
}
