import React, { useState, useEffect, useRef } from "react";

export default function Content() {

  // Estados para los LEDs
  const [stateLED1, setLED1On] = useState("disp");
  const [stateLED2, setLED2On] = useState("disp");


  // Estado para mostrar información recibida del JSON
//  const [JSONCorto, setJSONCorto] = useState<string>("");
//  const [JSONPolaridad, setJSONPolaridad] = useState<string>("");

  // Estado para el contenido de la caja de texto
  const [messages, setMessages] = useState<string>("");

  // Referencia al input 
  const inputRef = useRef<HTMLInputElement>(null);

  // Estado para la comunicación serie por puerto
  const [port1, setPort1] = useState<SerialPort | null>(null);
  const [port2, setPort2] = useState<SerialPort | null>(null);

  // Estado para el aviso de finalizado
  const [showMessage, setShowMessage] = useState(false);
  const [showMessageError, setShowMessageError] = useState(false);

  useEffect(() => {
  if (stateLED1 == "on" && stateLED2 == "on" ) {
    setShowMessage(true);   // Mostrar el aviso
  }}, [stateLED1, stateLED2]);

  useEffect(() => {
  if (stateLED1 == "off" || stateLED2 == "off") {
    setShowMessageError(true);   // Mostrar el aviso de error
  }}, [stateLED1, stateLED2]);


    // Cuando el componente carga, ponemos el foco en el input
  useEffect(() => {
    inputRef.current?.focus();
    }, []);


// Función para esperar N milisegundos
const delay = (ms: number) => new Promise(resolve => setTimeout(resolve, ms));

// Función para limpiar estados
const resetEstados = () => {
  setLED1On("disp");
  setLED2On("disp");
  setMessages("");
};

// Manejar cuando el GM65 “escribe” algo
const handleChange = (e: React.ChangeEvent<HTMLInputElement>) => {
  setMessages(e.target.value); // copiar lo que se escribió al input
};

// Manejar cuando el GM65 manda "Enter" después del código
const handleKeyDown = (e: React.KeyboardEvent<HTMLInputElement>) => {
if (e.key === "Enter") {
  console.log("Código leído:", e.currentTarget.value);

    // Mostrar en el input y limpiar valor
    //e.currentTarget.value = "";
    setMessages(e.currentTarget.value);

    // Esperar 30 segundos antes de borrar el input
    setTimeout(() => {
      setMessages(""); 
    }, 30000); // 10000ms = 10 segundos

}
};

const getColor1 = () => {
  switch (stateLED1) {
    case "on":
      return "limegreen"; // Enciende con 1
    case "waiting":
      return "yellow";    // Enciende con 2
    case "off":
      return "#f73d18ff"; // Enciende con 0
    default:
      return "#a193ff77"; 
  }
};

const getColor2 = () => {
  switch (stateLED2) {
    case "on":
      return "limegreen"; // Enciende con 1
    case "waiting":
      return "yellow";    // Enciende con 2
    case "off":
      return "#f73d18ff"; // Enciende con 0
    default:
      return "#a193ff77";    
  }
};



const connectDUALONE = async () => {
  try {
    // pedir puerto al usuario
    const selectedPort: SerialPort = await (navigator as any).serial.requestPort();
    await selectedPort.open({ baudRate: 115200 }); 

    // guardamos el puerto en el estado
    setPort1(selectedPort);
    console.log("Puerto COM Test conectado ");

    // empezamos a leer de Arduino
    readFromDual(selectedPort);
  } catch (err) {
    console.error("Error al conectar con la placa:", err);
  }
};

const connectCOMLoad = async () => {
  try {
    // pedir puerto al usuario
    const selectedPort: SerialPort = await (navigator as any).serial.requestPort();
    await selectedPort.open({ baudRate: 9600 }); 

    // guardamos el puerto en el estado
    setPort2(selectedPort);
    console.log("Puerto COM CargaV conectado");

    // empezamos a leer de Arduino
    readFromLoad(selectedPort);
  } catch (err) {
    console.error("Error al conectar con la placa:", err);
  }
};


const sendToPulsar = async (char: string) => {
 if (port1 && port1.writable){
    const encoder = new TextEncoder();
    const writer = port1.writable.getWriter();
    await writer.write(encoder.encode(char));
    writer.releaseLock();
    console.log("JSON enviado a PULSAR:", char); // <-- ahora sí imprime el dato enviado
 }
};

const sendToLoad = async (char: string) => {
  if (port2 && port2.writable) {
    const encoder = new TextEncoder();
    const writer = port2.writable.getWriter();
    await writer.write(encoder.encode(char + "\r\n"));  
    writer.releaseLock();
    console.log("✅ JSON enviado:", char);
  }
};

const readFromDual = async (selectedPort: SerialPort) => {
  const decoder = new TextDecoderStream();
  (selectedPort.readable as any).pipeTo(decoder.writable as any);
  const reader = decoder.readable.getReader();

  let buffer = "";

  while (true) {
    const { value, done } = await reader.read();
    if (done) break;

    if (value) {
      buffer += value; // acumulamos en el buffer

      let parts = buffer.split("\n");  // separamos por salto de línea
      buffer = parts.pop() || "";      // guardamos lo que quedó incompleto
      
      for (const part of parts){

        const trimmed = part.trim();
        if (!trimmed) continue; //evita parsear vacío

        try {
          const parsed = JSON.parse(trimmed);
          console.log("JSON recibido:", parsed); // Imprimes JSON en crudo

            if (parsed) {
            // 🔔 Emitimos evento con el JSON recibido
            window.dispatchEvent(new CustomEvent("jsonReceived", { detail: parsed }));
            if (parsed.Led1) setLED1On(parsed.Led1);
            if (parsed.Led2) setLED2On(parsed.Led2);
            }
        } 
        catch (err) {
          console.error("Error parseando JSON:", err, trimmed);
        }
      }
    }
  }
};


const readFromLoad = async (selectedPort: SerialPort) => {
  const decoder = new TextDecoderStream();
  (selectedPort.readable as any).pipeTo(decoder.writable as any);
  const reader = decoder.readable.getReader();

  let buffer = "";

  while (true) {
    const { value, done } = await reader.read();
    if (done) break;

    if (value) {
      buffer += value; // acumulamos en el buffer

      let parts = buffer.split("\n");  // separamos por salto de línea
      buffer = parts.pop() || "";      // guardamos lo que quedó incompleto
      
      for (const part of parts){

        const trimmed = part.trim();
        if (!trimmed) continue; //evita parsear vacío

        try {
          const parsed = JSON.parse(trimmed);
          console.log("JSON recibido:", parsed); // Imprimes JSON en crudo
          if (parsed) {
            window.dispatchEvent(new CustomEvent("jsonReceived", { detail: parsed }));
            if (parsed.Led2) setLED2On(parsed.Led2);
          }
        } 
        catch (err) {
          console.error("Error parseando JSON:", err, trimmed);
        }
      }
    }
  }
};




// --- Espera una respuesta JSON con "Result":"OK" ---
const waitForOK = async (): Promise<boolean> => {
  return new Promise((resolve) => {
    const timeout = setTimeout(() => {
      resolve(false); // ⏱️ No llegó OK en 2s
    }, 10000);

    const checkResponse = (event: CustomEvent) => {
      const parsed = event.detail;

      if (parsed.Result === "OK") {
        clearTimeout(timeout);
        window.removeEventListener("jsonReceived", checkResponse as any);
        resolve(true); // ✅ OK recibido
      }
    };
    window.addEventListener("jsonReceived", checkResponse as any);
  });
};


// --- Secuencia completa ---
const Arranque = async () => {
  if (!port1) {
      alert("Selecciona un puerto COM para el controlador.");
      return;
    }

  if (!port2) {
      alert("Selecciona un puerto COM para la carga variable");
      return;
    }

    // Rutina de ejecución
    try {

          sendToPulsar(JSON.stringify({ Function: "Enviar ID", ID: messages }) + "\n")  //Enviar a la pulsar ID 

          setLED1On("waiting");       // Espera LED1 Prueba Cortocircuito
          setLED2On("waiting");       // Espera LED2 Prueba Demanda Corriente
        
          // ===== Rutina de Prueba de Cortocircuito =====
          await runTest();

          // ===== Rutina de Prueba de Carga Variable =====
          await runCorto();     

          console.log("✅ Secuencia completada correctamente");
          await delay(10000);

  } catch (err) {
    console.error("❌ Error en la secuencia:", err);
  }
};



async function runTest(){
  try{
    await delay(6000);
    console.log("➡️ Configurando la carga a 3A...");
    await sendToLoad('{"Funcion": "DYN", "Config": {"Resolution": [3.1, 1, 3.1, 1], "Range": [50, 30]}, "Start": "CFG_ON"}');
    await delay(2000);

    console.log("➡️ Encendiendo la carga...");
    await sendToLoad('{"Funcion": "Other", "Config": {"Resolution": [], "Range": []}, "Start": "CFG_ON"}');
    await delay(3000);
    
    console.log("➡️ Midiendo demanda nominal...");
    await sendToPulsar(JSON.stringify({ Function: "Lectura Nom" }) + "\n");
    const OKDemandaC1 = await waitForOK(); // Confirmación de demanda 3A
    if (OKDemandaC1){
      console.log("✅ Demanda Nominal corto OK");
      setLED1On("on");       // Ok demanda nominal
      await delay(500);
      sendToPulsar(JSON.stringify({ Function: "TestDemanda", Estado: "OK" }) + "\n");
      await delay(2000);
    }
    else{
      console.log("❌ Falló demanda nominal");
      setLED1On("off");       // Error demanda nominal
      await sendToLoad('{"Funcion": "Other", "Config": {"Resolution": [], "Range": []}, "Start": "CFG_OFF"}');
      //Enviar datos de funcionamiento a la Pulsar para DB
      await delay(500);
      sendToPulsar(JSON.stringify({ Function: "TestDemanda", Estado: "FALLA" }) + "\n");
          await delay(2000);
      return;
    }
  }catch(error) {
    console.error("⚠️ Error en rutina:", error);
    setLED1On("off");
    setLED2On("off");
    await delay(500);
    await sendToLoad('{"Funcion": "Other", "Config": {"Resolution": [], "Range": []}, "Start": "CFG_OFF"}');
  }
}


// ⚡ Función para ejecutar la rutina de Carga Variable
async function runCorto() {
  try {
    // Inicio de Prueba de Corto
    console.log("➡️ Enviando Cortocircuito...");
    await delay(100);
    await sendToPulsar(JSON.stringify({ Function: "Cortocircuito" }) + "\n");
    const OKCorto = await waitForOK(); // Confirmación de demanda 0A
    if (OKCorto){
      console.log("✅ Protección en corto OK");
    }
    else{
      console.log("❌ Falló en protección de corto");
      setLED2On("off");       // Error demanda corto
      await delay(500);
      await sendToLoad('{"Funcion": "Other", "Config": {"Resolution": [], "Range": []}, "Start": "CFG_OFF"}');
      //Enviar datos de funcionamiento a la Pulsar para DB
      await delay(500);
      sendToPulsar(JSON.stringify({ Function: "TestCorto", Estado: "FALLA" }) + "\n");          
      return;
    }
    

    await delay(3000);
    console.log("➡️ Midiendo demanda...");
    await sendToPulsar(JSON.stringify({ Function: "Lectura Nom" }) + "\n");
    const OKDemandaC2 = await waitForOK(); // Confirmación de demanda 3A
    if (OKDemandaC2){
      console.log("✅ Demanda después de corto OK");
      setLED2On("on");       // Prueba aprobada
      //Enviar datos de funcionamiento a la Pulsar para DB
      await delay(500);
      sendToPulsar(JSON.stringify({ Function: "TestCorto", Estado: "OK" }) + "\n");
    }
    else{
      console.log("❌ Falló demanda después de corto");
      setLED2On("off");       // Error demanda nominal
      await delay(500);
      await sendToLoad('{"Funcion": "Other", "Config": {"Resolution": [], "Range": []}, "Start": "CFG_OFF"}');
      //Enviar datos de funcionamiento a la Pulsar para DB
      await delay(500);
      sendToPulsar(JSON.stringify({ Function: "TestCorto", Estado: "FALLA" }) + "\n");
      return;
    }

    await delay(500);
    console.log("➡️ Apagando la carga...");
    await sendToLoad('{"Funcion": "Other", "Config": {"Resolution": [], "Range": []}, "Start": "CFG_OFF"}');

  } catch (error) {
    console.error("⚠️ Error en rutina Carga Variable:", error);
    setLED2On("off");
    await sendToLoad('{"Funcion": "Other", "Config": {"Resolution": [], "Range": []}, "Start": "CFG_OFF"}');
    sendToPulsar(JSON.stringify({ Function: "TestCarga", Estado: "FALLA" }) + "\n");
  }
}



  return (
    <main
      style={{
        width: "565px",        // ancho fijo
        margin: "0 auto",      // centra horizontalmente
        flex: 1,
        display: "flex",
        flexDirection: "column",
        gap: "20px",
        padding: "20px",
      }}
    >


    {/* Primera Sección: ID de producto */}
    <div
        style={{
            display: "flex",       // alineación en fila
            alignItems: "center",  // centrado vertical
            gap: "10px",           // espacio entre elementos
        }}
        >
        <h2 style={{ margin:  0}}>ID del producto</h2>

        <input
            ref={inputRef}
            type= "text"
            value={messages}
            onChange={handleChange}
            onKeyDown={handleKeyDown}
            style={{ width: "150px", height: "25px" }}
        />

        {/*COM para Pulsar Control*/}
        <button
        onClick={connectDUALONE}
        style={{
            padding: "10px",
            width: "100px",
            borderRadius: "5px",
            cursor: "pointer",
            backgroundColor: port1 ? "#4CAF50" : "#e4e4e490", // Verde si conectado
            color: port1 ? "white" : "black",
            fontWeight: port1 ? "bold" : "normal",
        }}
        >
        {port1 ? "COM TEST ✅" : "Seleccionar COM TEST"}
        </button>

        {/*COM para Pulsar Potencia*/}
        <button
        onClick={connectCOMLoad}
        style={{
            padding: "10px",
            width: "100px",
            borderRadius: "5px",
            cursor: "pointer",
            backgroundColor: port2 ? "#4CAF50" : "#e4e4e490", // Verde si conectado
            color: port2 ? "white" : "black",
            fontWeight: port2 ? "bold" : "normal",
        }}
        >
        {port2 ? "COM CVar ✅" : "Seleccionar COM CVar"}
        </button>


    </div>

    {/* Segunda Sección: Tablero de indicadores*/}
    <div
    style={{
        display: "flex",
        flexDirection: "row",   // o "row" si quieres en fila
        justifyContent: "center",  // centra verticalmente
        alignItems: "center",      // centra horizontalmente
        border: "2px solid #333",       // borde oscuro
        borderRadius: "10px",           // esquinas redondeadas
        padding: "40px",                // espacio interno
        backgroundColor: "#f5f5f5",     // color de fondo claro
        boxShadow: "2px 2px 10px rgba(0,0,0,0.2)", // sombra para dar relieve
        width: "490px",                 // ancho fijo opcional
        height: "1px",                 // ancho fijo opcional
    }}
    >

    <h3
     style={{         
        alignItems: "center", 
        marginTop: 20 }}

    >Tablero de indicadores 🔎 </h3>

    {/* Configuración Fila 1 */}
    <div style={{ 
        display: "flex", 
        alignItems: "center", 
        gap: "12px" }}>

    <div
    style={{
        width: "20px",
        height: "20px",
        borderRadius: "50%",
        backgroundColor: "limegreen",
        border: "1px solid #333",
    }}
    ></div>
    <span>Prueba aprobada</span>
    </div>

    {/* Configuración Fila 2 */}
    <div style={{ 
        display: "flex", 
        alignItems: "center", 
        gap: "12px" }}>
    <div
    style={{
        width: "20px",
        height: "20px",
        borderRadius: "50%",
        backgroundColor: "red",
        border: "1px solid #333",
    }}
    ></div>
    <span>Prueba fallida</span>
    </div>

    {/* Configuración Fila 3 */}
    <div style={{ 
        display: "flex", 
        alignItems: "center", 
        gap: "12px" }}>
    <div
    style={{
        width: "20px",
        height: "20px",
        borderRadius: "50%",
        backgroundColor: "yellow",
        border: "1px solid #333",
    }}
    ></div>
    <span>Espera...</span>
    </div>
    </div> {/*div Final de Sección */}



    {/* Tercera Sección: Botones e indicadores LEDS */}
    <div
    style={{
        flex: 1,
        display: "flex", // dividir en columnas
        justifyContent: "center",
        alignItems: "flex-start",
        gap: "10px",
        padding: "5px",
    }}
    >

    {/* Columna izquierda: Botones de Prueba*/}
    <div
        style={{
        flex: 1,
        display: "flex",
        flexDirection: "column",
        alignItems: "center",
        gap: "10px",
        }}
    >
    <h3>PRUEBA TEST</h3>

    <h3
    style={{ margin:  2}}
    >Corriente Nom</h3>

    <h3
    style={{ margin:  10}}
    >Cortocircuito</h3>

    {/*Botón de activación de Prueba de Cortocircuito
    <button onClick={() => sendToDual("{\"Function\":\"Cortocircuito\"}")}
        style={{
        padding: "10px",
        width: "120px",
        borderRadius: "5px",
        cursor: "pointer",
        backgroundColor: "#e4e4e490",
        }}
        >
    {stateLED1 ? "Cortocircuito" : "Cortocircuito"}
    </button> */}

    {/*Botón de activación de Prueba de Polaridad
    <button onClick={() => sendToDual("{\"Function\":\"Polaridad\"}")}
        style={{
        padding: "10px",
        width: "120px",
        borderRadius: "5px",
        cursor: "pointer",
        backgroundColor: "#e4e4e490",
        }}
        >
    {stateLED2 ? "Polaridad" : "Polaridad"}
    </button>*/}

    </div>

    {/* Columna derecha: LEDs indicadores */}
    <div
        style={{
        flex: 1,
        display: "flex",
        flexDirection: "column",
        justifyContent: "center",
        alignItems: "center",
        gap: "12px",
        padding: "0px",
        }}
    >
    <h3>ESTADO</h3>

    {/* LED Cortocirtuito */}
    <div
    style={{
        width: "30px",
        height: "30px",
        borderRadius: "50%",
        backgroundColor: getColor1(),
        border: "2px solid #333",
        marginBottom: "5px",
        transition: "background-color 0.5s ease" // Transición
    }}
    ></div>

    {/* LED Polaridad */}
    <div
    style={{
        width: "30px",
        height: "30px",
        borderRadius: "50%",
        backgroundColor: getColor2(),
        border: "2px solid #333",
        marginBottom: "5px",
        transition: "background-color 0.5s ease" // Transición
    }}
    ></div>

    </div>

    {/* Columna derecha: Botones de control */}
    <div style={{ 
            flex: 1, 
            display: "flex", 
            flexDirection: "column", 
            justifyContent: "center",
            alignItems: "center",
            marginTop: "50px",
            gap: "10px" }}>

    <button onClick={(Arranque)}
    style={{
        padding: "10px",
        width: "120px",
        height: "50px",
        alignContent: "center",
        borderRadius: "5px",
        cursor: "pointer",
        backgroundColor: "#7326ef9f", 
        border: "1px solid #000000ff",
        }}
    >
    ARRANQUE
    </button>

    <button onClick={(resetEstados)}
    style={{
        padding: "10px",
        width: "120px",
        height: "50px",
        alignContent: "center",
        borderRadius: "5px",
        cursor: "pointer",
        backgroundColor: "#fcbb3ac7", 
        border: "1px solid #000000ff",
        }}
    >
    RESET
    </button>
    </div>
    </div>


    {/*Cuarta Sección*/}
    <div
    style={{
        flex: 1,
        display: "flex", // dividir en columnas
        justifyContent: "center",
        alignItems: "flex-start",
        gap: "10px",
        padding: "5px",
    }}
    >
    </div>


    {/* Mensaje de Aprobación*/}
    {showMessage && (
    <div
        style={{
        position: "fixed",
        top: 0,
        left: 0,
        width: "100%",
        height: "100%",
        backgroundColor: "rgba(0,0,0,0.6)", // fondo semi-transparente
        display: "flex",
        justifyContent: "center",
        alignItems: "center",
        zIndex: 1000,
        }}
    >
        <div
        style={{
            backgroundColor: "white",
            padding: "30px",
            borderRadius: "15px",
            textAlign: "center",
            boxShadow: "0 5px 20px rgba(0,0,0,0.3)",
            animation: "fadeIn 0.5s ease", // animación opcional
        }}
        >
        <h2 style={{ color: "green" }}>✅ ¡El producto cumple las especificaciones! ✅</h2>
        <button
        onClick={() => {
            setShowMessage(false);   // Cierra el aviso
            setLED1On("disp");        // Apaga LED1 Prueba Cortocircuito
            setLED2On("disp");        // Apaga LED2 Prueba Polaridad
            setMessages("");         // Limpia casillas
          //  setJSONCorto("");        // Limpia JSON
         //   setJSONPolaridad(""); 
        }}
        >
        Cerrar
        </button>
        </div>
    </div>
    )}


    {/* Mensaje de Error*/}
    {showMessageError && (
    <div
        style={{
        position: "fixed",
        top: 0,
        left: 0,
        width: "100%",
        height: "100%",
        backgroundColor: "rgba(0,0,0,0.6)", // fondo semi-transparente
        display: "flex",
        justifyContent: "center",
        alignItems: "center",
        zIndex: 1000,
        }}
    >
        <div
        style={{
            backgroundColor: "white",
            padding: "30px",
            borderRadius: "15px",
            textAlign: "center",
            boxShadow: "0 5px 20px rgba(0,0,0,0.3)",
            animation: "fadeIn 0.5s ease", // animación opcional
        }}
        >
        <h2 style={{ color: "red" }}>❌ ¡El producto presenta fallas! ❌</h2>
        <button
        onClick={() => {
            setShowMessageError(false);   // Cierra el aviso
            setMessages("");         // Limpia casillas
        }}
        >
        Cerrar
        </button>
        </div>
    </div>
    )}

    </main>
  );
}
