require 'sinatra'
require 'twilio-ruby'
require 'pusher'
require 'rack/throttle'
require 'memcached'

#Memcached config
CACHE = Memcached.new
PREFIX = :throttle

# User Rack Throttle to limit requests
use Rack::Throttle::Hourly,   :cache => CACHE, :code => 200, :key_prefix => PREFIX,  :max => 10   # requests
use Rack::Throttle::Interval, :cache => CACHE, :code => 200, :key_prefix => PREFIX,  :min => 30.0   # seconds

before do
  # Setup 
  Pusher.app_id = ENV['GARDUINO_APP_ID']
  Pusher.key = ENV['GARDUINO_KEY']
  Pusher.secret = ENV['GARDUINO_SECRET']

  @twilio_number = ENV['TWILIO_NUMBER']
  @my_cellphone = ENV['PERSONAL_PHONE']
  @client = Twilio::REST::Client.new ENV['TWILIO_ACCOUNT_SID'], ENV['TWILIO_AUTH_TOKEN']
end

get "/" do
  erb :index
end

def sendMessage(to, body)
  message = @client.account.messages.create(
    :from => @twilio_number,
    :to => to,
    :body => body
  )
  puts message.to
end

get '/arduino-data?*' do
  alert = params[:alert]
  temp = params[:temp]
  moisture = params[:moisture]
  if alert
    if moisture
      body = "Garden Alert: #{alert}. Water level: #{moisture}."
    else
      body = "Garden Alert: #{alert}. Temperature: #{temp}."
    end
  else
    body = "Garden Moisture: #{moisture}. Garden Temp: #{temp}."
  end
  
  sendMessage('2066505813', body)
end

get '/check-status?*' do
  puts Pusher['garduino'].trigger('status', {:message => ''})
end