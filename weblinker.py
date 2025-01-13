from dash import Dash, html,dcc,callback,Input,Output
import plotly.express as px
import pandas as pd
import subprocess
import threading
import os
from threading import Timer
import webbrowser
script_process = None
process_lock = threading.Lock()
app = Dash()

app.layout = [
    html.Div(style={'textAlign': 'center', 'fontFamily': 'Arial'},
    children=[
        html.H1("Pressure Sensor Data"),
        html.Button("Run Script", id="start-button", n_clicks=0, style={'fontSize': '18px', 'padding': '10px'}),
        html.Button("Stop Script", id="stop-button", n_clicks=0, style={'fontSize': '18px', 'padding': '10px', 'marginLeft': '10px'}),
        html.Div(id="output", style={'marginTop': '20px', 'fontSize': '16px', 'color': 'green'}),
    ]),
    dcc.Graph(id = 'live-update-graph'),
    dcc.Interval(
        id='interval-component',
        interval=1000,
        n_intervals=0
    )
]
def open_browser():
    if not os.environ.get("WERKZEUG_RUN_MAIN"):
        webbrowser.open_new('http://127.0.0.1:8050/')
@app.callback(
    Output("output", "children"),
    Input("start-button", "n_clicks"),
    prevent_initial_call=True,
    running=[(Output("start-button", "disabled"), True, False)]
)
def start_script(n_clicks):
    global script_process
    with process_lock:
        if script_process and script_process.poll() is None:
            return "Script is already running!"
        
        try:
            # Start the script as a subprocess
            script_process = subprocess.Popen(
                ["python", "pres.py"],  # Replace with the path to your script
                stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True
            )
            return "Script started successfully!"
        except Exception as e:
            return f"Failed to start the script: {str(e)}"

# Callback to handle "Stop Script" button
@app.callback(
    Output("output", "children", allow_duplicate=True),
    [Input("stop-button", "n_clicks")],
    prevent_initial_call=True
)
def stop_script(n_clicks):
    global script_process
    with process_lock:
        if script_process and script_process.poll() is None:
            script_process.terminate()
            script_process.wait()  # Ensure the process has stopped
            return "Script stopped successfully!"
        return "No script is running to stop."

@callback(Output('live-update-graph','figure'),Input('interval-component','n_intervals'))
def update_graph_live(n):
    try:
        df = pd.read_csv('data.csv')
        df.columns = ["Time","Pressure"]        
        fig = px.line(df.tail(20),x='Time',y='Pressure',title='Pressure (cmH20)')
        fig.update_layout(yaxis_range = [-1200,1200])
        return fig
    except:
        return {}
    

if __name__ == '__main__':
    Timer(1, open_browser).start()
    app.run_server(debug=True, port=8050)