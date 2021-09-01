from flask import Flask
from jinja2 import Environment, FileSystemLoader, select_autoescape

app = Flask(__name__)
env = Environment(loader=FileSystemLoader(""), autoescape=select_autoescape)

@app.route("/")
def index():
  template = env.get_template('recording/monitor/index.html')
  return template.render()

if __name__ == "__main__":
  app.run()
