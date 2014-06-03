require 'sinatra'
require 'twilio-ruby'
require 'pusher'

before do
  # Setup 
  Pusher.app_id = ENV['GARDUINO_APP_ID']
  Pusher.key = ENV['GARDUINO_KEY']
  Pusher.secret = ENV['GARDUINO_SECRET']

  @twilio_number = ENV['TWILIO_NUMBER']
  @my_cellphone = ENV['PERSONAL_PHONE']
  @client = Twilio::REST::Client.new ENV['TWILIO_ACCOUNT_SID'], ENV['TWILIO_AUTH_TOKEN']
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
  
  message = @client.account.messages.create(
    :from => @twilio_number,
    :to => '2066505813',
    :body => body
  )
  puts message.to
end

get '/check-status?*' do
  puts Pusher['garduino'].trigger('status', {:message => ''})
end